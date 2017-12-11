gcc -g3 setup.c -lpcg_random -o setupCounterfactualAnalysis -lgsl -lgslcblas -lm
gcc -g3 no_intervention.c -lpcg_random -o noIntervention -lgsl -lgslcblas -lm
gcc -g3 vaccination.c -lpcg_random -o vaccination -lgsl -lgslcblas -lm
gcc -g3 quarantine.c -lpcg_random -o quarantine -lgsl -lgslcblas -lm
gcc -g3 social_distancing.c -lpcg_random -o social_distancing -lgsl -lgslcblas -lm

./setupCounterfactualAnalysis
./noIntervention
./quarantine
./vaccination
./social_distancing
