library(ggplot2)

base.dir = '~/svn/counterfactual'
setwd(paste(base.dir,'output',sep='/'))
aggregate = function(a,var){
  apply(a,2,function(x){sum(x==var)})
}

plotResults <- function(filename){
  a = read.csv(filename,header=FALSE)
  a = a[,-ncol(a)]
  states = unique(unlist(a))
  data = as.data.frame(sapply(states,function(s){aggregate(a,s)}))
  data$t = 1:nrow(data)
  # data = data.frame(
  #   t = 1:ncol(a),
  #   S = aggregate(a,0),
  #   I = aggregate(a,1),
  #   R = aggregate(a,2),
  #   Q = aggregate(a,3),
  #   V = aggregate(a,4)
  # )
  plt = ggplot(data,aes(x=t))
  for(col in colnames(data)[startsWith(colnames(data),"V")]){
    plt = plt + geom_line(aes_(y=as.name(col),col=col))
  }
  pdf(paste(filename,'pdf',sep='.'))
  print(plt)
  dev.off()
  return(plt)
}

files = list.files()
data = list()
for(file in files){
  data[[file]] = read.csv(file,header = FALSE)
  data[[file]] = data[[file]][,-ncol(data[[file]])]
}
same_run = data[startsWith(prefix = 'no_intervention.0',names(data))]
different_run = data[startsWith(prefix = 'no_intervention.1',names(data))]
intervention = data[startsWith(prefix = 'intervention',names(data))]

intervention = array(
  abind::abind(intervention),
  c(nrow(intervention[[1]]),ncol(intervention[[2]]),length(intervention))
)
intervention.names = unique(as.vector(intervention))
intervention = array(
  sapply(
    intervention.names,
    function(name){
      apply(
        intervention,
        c(2,3),
        function(x){
          sum(x==name)
        }
      )
    }
  ),
  c(
    dim(intervention)[-1],
    length(intervention.names)
  )
)
dimnames(intervention) <- list(NULL,NULL,intervention.names)
different_run = array(
  abind::abind(different_run),
  c(nrow(different_run[[1]]),ncol(different_run[[2]]),length(different_run))
)
different_run.names = unique(as.vector(different_run))
different_run = array(
  sapply(
    different_run.names,
    function(name){
      apply(
        different_run,
        c(2,3),
        function(x){
          sum(x==name)
        }
      )
    }
  ),
  c(
    dim(different_run)[-1],
    length(different_run.names)
  )
)
dimnames(different_run) <- list(NULL,NULL,different_run.names)

same_run = array(
  abind::abind(same_run),
  c(nrow(same_run[[1]]),ncol(same_run[[2]]),length(same_run))
)
same_run.names = unique(as.vector(same_run))
same_run = array(
  sapply(
    same_run.names,
    function(name){
      apply(
        same_run,
        c(2,3),
        function(x){
          sum(x==name)
        }
      )
    }
  ),
  c(
    dim(same_run)[-1],
    length(same_run.names)
  )
)
dimnames(same_run) <- list(NULL,NULL,same_run.names)

control.names = intersect(intervention.names,different_run.names)
experimental.names = intersect(intervention.names,same_run.names)

experimental =
  intervention[,,paste(experimental.names)] - same_run[,,paste(experimental.names),drop=FALSE]
experimental = abind::abind(
  experimental,intervention[,,!(intervention.names %in% experimental.names),drop=FALSE],along=3
)
experimental = abind::abind(
  experimental,intervention[,,!(same_run.names %in% experimental.names),drop=FALSE],along=3
)
control = intervention[,,paste(control.names)] - different_run[,,paste(control.names)]
control = abind::abind(
  control,intervention[,,!(intervention.names %in% control.names),drop=FALSE],along=3
)
control = abind::abind(
  control,intervention[,,!(different_run.names %in% control.names),drop=FALSE],along=3
)

quantile.control = apply(control,c(1,3),function(x){quantile(x,c(.025,.5,.975))})
quantile.experimental = apply(experimental,c(1,3),function(x){quantile(x,c(.025,.5,.975))})

melted.experimental = melt(experimental)
melted.control = melt(control)

melted.quantile.experimental = melt(quantile.experimental)
melted.quantile.control = melt(quantile.control)

print(quantile.control[,nrow(control),])
print(quantile.experimental[,nrow(experimental),])


experimental.plt = ggplot() +
  geom_point(
    data = melted.experimental,
    aes(x=Var1,col=as.character(Var3),y=value,group = Var2+Var3),
    alpha=.01
  ) +
  ylim(min(melted.control$value),max(melted.control$value))
control.plt = ggplot() +
  geom_point(
    data = melted.control,
    aes(x=Var1,col=as.character(Var3),y=value,group = Var2+Var3),
    alpha=.01) +
  ylim(min(melted.control$value),max(melted.control$value))

ggplot() +
  geom_line(
    data = filter(melted.quantile.experimental,Var1=="50%"),
    aes(x=Var2,col=as.character(Var3),y=value)
  ) +
  geom_ribbon(
    data = tidyr::spread(filter(melted.quantile.experimental,Var1!='50%'),Var1,value),
    aes(x=Var2,col=as.character(Var3),fill=as.character(Var3),ymin=`2.5%`,ymax=`97.5%`),
    alpha=.1) +
  ylim(
    min(min(melted.quantile.experimental$value),min(melted.quantile.control$value)),
    max(max(melted.quantile.experimental$value),max(melted.quantile.control$value))
  )
ggplot() +
  geom_line(
    data = filter(melted.quantile.control,Var1=='50%'),
    aes(x=Var2,col=as.character(Var3),y=value)
  ) +
  geom_ribbon(
    data = tidyr::spread(filter(melted.quantile.control,Var1!='50%'),Var1,value),
    aes(x=Var2,col=as.character(Var3),fill=as.character(Var3),ymin=`2.5%`,ymax=`97.5%`),
    alpha=.1) +
  ylim(
    min(min(melted.quantile.experimental$value),min(melted.quantile.control$value)),
    max(max(melted.quantile.experimental$value),max(melted.quantile.control$value))
  )
