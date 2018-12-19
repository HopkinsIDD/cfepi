######################################################################
###                          Counterfactual                        ###
######################################################################
######################################################################

################################################################################
####Setup Test Functions
################################################################################
# context('Setup')


rm(list =ls())
options(warn=2)

output_directory = tempdir()

test_that('Creating output directory for testing',{
  expect_true(dir.exists(output_directory))
})
##test of initialization method.
setup_counterfactual_working = test_that('setup_counterfactual works',{
  trans <- matrix(0,3,3)
  inter <- matrix(0,3,3)
  trans[3,2] <- .2
  inter[2,1] <- .4
  init <- c(3990,10,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 2
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),NA)
  expect_equal({length(list.files(output_directory))},4)
  file.remove(paste(output_directory,list.files(output_directory),sep='/'))
  expect_equal({length(list.files(output_directory))},0)
})

test_that('setup_counterfactual works in boundary cases',{
  trans <- matrix(0,3,3)
  inter <- matrix(0,3,3)
  trans[3,2] <- .2
  init <- c(3990,10,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 1
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),NA)
  expect_equal({length(list.files(output_directory))},2)
  file.remove(paste(output_directory,list.files(output_directory),sep='/'))
  expect_equal({length(list.files(output_directory))},0)

  trans <- matrix(0,3,3)
  inter <- matrix(0,3,3)
  inter[2,1] <- .2
  init <- c(3990,10,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 1
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),NA)
  expect_equal({length(list.files(output_directory))},2)
  file.remove(paste(output_directory,list.files(output_directory),sep='/'))
  expect_equal({length(list.files(output_directory))},0)

  trans <- matrix(0,3,3)
  inter <- matrix(0,3,3)
  trans[3,2] <- .2
  inter[2,1] <- .4
  init <- c(0,0,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 1
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),NA)
  expect_equal({length(list.files(output_directory))},2)
  file.remove(paste(output_directory,list.files(output_directory),sep='/'))
  expect_equal({length(list.files(output_directory))},0)
})

test_that('setup_counterfactual throws errors if directory doesn\'t exist',{
  trans <- matrix(0,3,3)
  inter <- matrix(0,3,3)
  trans[3,2] <- .2
  inter[2,1] <- .4
  init <- c(3990,10,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 1
  expect_error(setup_counterfactual(
    'fake_folder/test_counterfactual',
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),"Could not open file fake_folder/test_counterfactual.t.0.0.dat\n")
})

test_that('setup_counterfactual throws errors if transition matrix is wrong size',{
  inter <- matrix(0,3,3)
  inter[2,1] <- .4
  init <- c(3990,10,0)
  # init <- c(390,10,0)
  npop <- sum(init)
  inter <- inter/npop
  ntime <- 365
  ntrial <- 1
  trans <- matrix(0,3,2)
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),"Dimension mismatch.  The transition matrix should have one col for each variable.\n")
  trans <- matrix(0,2,3)
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),"Dimension mismatch.  The transition matrix should have one row for each variable.\n")
  trans <- matrix(0,4,3)
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),"Dimension mismatch.  The transition matrix should have one row for each variable.\n")
  trans <- matrix(0,3,4)
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),"Dimension mismatch.  The transition matrix should have one col for each variable.\n")
})

test_that("Files were removed",{
  expect_equal({length(list.files(output_directory))},0)
})

context('run: general')
test_that('run_scenario works',{
  if(!setup_counterfactual_working){
    skip("run_scenario relies on setup counterfactual.")
  }
  expect_error(setup_counterfactual(
    paste(output_directory,'test_counterfactual',sep='/'),
    init,
    inter,
    trans,
    ntime,
    ntrial
  ),NA)
  expect_equal({length(list.files(output_directory))},2*ntrial)
  expect_error({
      run_scenario(paste(output_directory,'test_counterfactual',sep='/'),'test-intervention',init,"None","None",list(),list(),ntime,ntrial)
    },NA
  )
  expect_equal({length(list.files(output_directory))},3*ntrial)
  file.remove(paste(output_directory,list.files(output_directory),sep='/'))
  expect_equal({length(list.files(output_directory))},0)
})
