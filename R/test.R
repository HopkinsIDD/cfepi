construct_initial_conditions <- function(vals) {
  ncompartments <- length(vals)
  compartments <- names(vals)
  return(
    matrix(unlist(mapply(n = names(vals), v = vals, function(n, v) {
      rep(
        sapply(compartments, function(x) {
          x == n
        }),
        times = v
      )
    })), ncol = ncompartments, byrow = TRUE, dimnames = list(NULL, compartments))
  )
}

generate_event_range <- function(current_state, event_type) {
  potential_candidates <- lapply(event_type[["preconditions"]], function(x) {
    return(which(apply(current_state[, x, drop = FALSE], 1, any)))
  })
  index <- 0
  dimension <- sapply(potential_candidates, length)
  finished <- FALSE
  return(function(k=1) {
    index <<- index + k
    array_index <- arrayInd(index, .dim = dimension)
    if (index > prod(dimension)) {
      finished <<- TRUE
    }
    if (finished) {
      return(NULL)
    }
    return(mapply(
      i = array_index,
      pc = potential_candidates,
      function(pc, i) {
        pc[i]
      }
    ))
  })
}

iota <- function(start=1) {
  iota_value <- start
  rc <- function(k = 1) {
    rc <- iota_value + k - 1
    iota_value <<- iota_value + k
    return(rc)
  }
  return(rc)
}

transform <- function(range, f) {
  return(function(k=1) {
    f(range(k))
  })
}

merge <- function(...) {
  sub_ranges <- list(...)
  sub_activate_by_index <- function(index, k=1) {
    return(sub_ranges[[index]](k))
  }
  currents <- sapply(seq_len(length(sub_ranges)), sub_activate_by_index)
  rc_sub <- function() {
    outer_indices <- which(currents == min(currents))
    rc <- currents[[outer_indices[[1]]]]
    currents[outer_indices] <<- sapply(outer_indices, sub_activate_by_index)
    return(rc)
  }
  rc <- function(k=1) {
    for (i in seq_len(k - 1)) {
      rc_sub()
    }
    return(rc_sub())
  }
}

filter <- function(input_range, f) {
  return(
    function(k=1) {
      for (i in seq_len(k)) {
        done <- FALSE
        while (!done) {
          rc <- input_range(1)
          done <- f(rc)
        }
      }
      return(rc)
    }
  )
}

sample <- function(input_range, probability, deterministic = FALSE) {
  if (deterministic) {
    stride_fun <- function(k) {
      return(ceiling(k * 1 / probability))
    }
  } else {
    stride_fun <- function(k) {
      return(sum(rgeom(k, probability) + 1))
    }
  }
  return(function(k = 1) {
    stride <- stride_fun(k)
    return(input_range(stride))
  })
}

repeat_n <- function(input_range, n = 1) {
  sub_index <- -1
  sub_value <- input_range()
  return(function(k = 1) {
    sub_k <- (sub_index + k) %/% n
    if (sub_k > 0) {
      sub_value <<- input_range(sub_k)
    }
    sub_index <<- sub_index + k - sub_k * n
    return(list(sub_index + 1, sub_value))
  })
}

library(magrittr)

test_run <- function(a, b, c) {
  a_range <- iota() %>%
    transform(function(x) {
      return(x * a)
    })
  b_range <- iota() %>%
    transform(function(x) {
      return(x * b)
    })
  c_range <- iota() %>%
    transform(function(x) {
      return(x * c)
    })
  full <- merge(a_range, b_range, c_range) %>%
    sample(probability = .1, deterministic = TRUE) %>%
    repeat_n(n = 3) %>%
    filter(function(x) {
      index <- x[[1]]
      value <- x[[2]]
      if (index == 0) {
        return(((value %/% b) %% 2) == 0)
      }
      if (index == 1) {
        return(((value %/% c) %% 2) == 0)
      }
      if (index == 2) {
        return(((value %/% a) %% 2) == 0)
      }
    })
  return(full)
}

satisfies <- function(proposed_state, event_type) {
  indices <- seq_len(length(event_type$preconditions))
  return(all(sapply(
    indices,
    function(x) {
      return(any(proposed_state[, event_type$preconditions[[x]]]))
    }
  )))
}

apply_event_entered <- function(state, index, event_type) {
  nonnull_postconditions <- !sapply(event_type[["postconditions"]], is.null)
  state[
    index[nonnull_postconditions],
    unlist(event_type[["postconditions"]])
  ] <- TRUE
  return(state)
}

apply_event_remained <- function(state, index, event_type) {
  nonnull_postconditions <- !sapply(event_type[["postconditions"]], is.null)
  state[
    index[nonnull_postconditions],
    unlist(event_type[["preconditions"]][nonnull_postconditions])
  ] <- FALSE
  return(state)
}

default_population_size <- 1e2
do_run <- function(
  initial_conditions = construct_initial_conditions(c(S = default_population_size - 1, I = 1, R = 0)),
  all_event_types = list(
    infection = list(
      name = "infection",
      preconditions = list("S", "I"),
      postconditions = list("I", NULL),
      rate = .2 / default_population_size
    ),
    recovery = list(
      name = "recovery",
      preconditions = list("I"),
      postconditions = list("R"),
      rate = .1
    )
  ),
  filter_list = list(
    function(x) {
      if (length(x) > 1) {
        return(runif(1, 0, 1) > .9)
      }
      return(TRUE)
    },
    function(x) {
      return(TRUE)
    }
  ),
  time_length = 365
) {
  future_states_remained <- lapply(filter_list, function(x) {
    return(initial_conditions)
  })
  future_states_entered <- lapply(filter_list, function(x) {
    return(initial_conditions & FALSE)
  })
  for (time in seq_len(time_length)) {
    current_states <- mapply(
      x = future_states_remained,
      y = future_states_entered,
      function(x, y) {
        return(x | y)
      },
      SIMPLIFY = FALSE
    )
    current_state_union <- Reduce(
      function(x, y) {
        x | y
      },
      x = current_states
    )
    future_states_remained <- current_states
    future_states_entered <- lapply(current_states, function(x) {
      x & FALSE
    })
    for (event_type in all_event_types) {
      event_range <- generate_event_range(event_type, current_state = current_state_union) %>%
        sample(probability = event_type[["rate"]]) %>%
        repeat_n(length(future_states_remained)) %>%
        filter(function(x) {
          filter_list[[x[[1]]]](x[[2]])
        }) %>%
        transform(function(x) {
          if (is.null(x[[2]])) {
            return()
          }
          current_state <- current_states[[x[[1]]]][x[[2]], , drop = FALSE]
          if (satisfies(current_state, event_type)) {
            future_states_entered[[x[[1]]]] <<- apply_event_entered(
              future_states_entered[[x[[1]]]],
              x[[2]],
              event_type
            )
            future_states_remained[[x[[1]]]] <<- apply_event_remained(
              future_states_remained[[x[[1]]]],
              x[[2]],
              event_type
            )
          }
          future_current_state_union <- Reduce(
            mapply(
              x = future_states_entered,
              y = future_states_remained,
              function(x, y) {
                x | y
              },
              SIMPLIFY = FALSE
            ),
            f = function(x, y) {
              x | y
            })

          if (!all(apply(future_current_state_union, 1, any))) {
            browser()
          }
          return(x)
        })

      event <- event_range()
      counter <- 0
      while (!is.null(event)) {
        counter <- counter + 1
        event <- event_range()
      }
      print(paste("time", time, "event type", event_type[["name"]], "count", counter))
    }
  }
  return(current_states)
}
