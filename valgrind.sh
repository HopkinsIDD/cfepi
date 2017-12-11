#!/bin/bash

#$ -l h_vmem=20G
#$ -l h_rt=100:00:00  
#$ -pe nodes 1
#$ -N counterfactual

# DIR=/home/jkaminsky/svn/counterfactual
DIR=~/svn/counterfactual
cd $DIR
gcc -g3 $DIR/multiple_trials.c -lgsl -lgslcblas -lm -lpcg_random -o $DIR/multipleTrials

valgrind --track-origins=yes --leak-check=full $DIR/multipleTrials
# valgrind --leak-check=full --track-origins=yes $DIR/multipleTrials
# gprof ./setupCounterfactualAnalysis
