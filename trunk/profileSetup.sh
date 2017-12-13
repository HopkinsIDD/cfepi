#!/bin/bash
#$ -l h_vmem=20G
#$ -l h_rt=100:00:00  
#$ -pe nodes 1
#$ -N counterfactual

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`
cd $SCRIPTPATH

gcc -pg -g3 -Og $SCRIPTPATH/src/multiple_trials.c -lgsl -lgslcblas -lm -lpcg_random -o $SCRIPTPATH/multipleTrials

$SCRIPTPATH/multipleTrials
