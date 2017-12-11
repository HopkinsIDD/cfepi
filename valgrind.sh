#!/bin/bash
#$ -l h_vmem=20G
#$ -l h_rt=100:00:00  
#$ -pe nodes 1
#$ -N counterfactual

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`
cd $SCRIPTPATH

gcc -g3 $SCRIPTPATH/multiple_trials.c -lgsl -lgslcblas -lm -lpcg_random -o $SCRIPTPATH/multipleTrials

valgrind --track-origins=yes --leak-check=full $SCRIPTPATH/multipleTrials
