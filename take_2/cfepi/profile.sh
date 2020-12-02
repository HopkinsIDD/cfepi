FLAMEGRAPH_REPO=~/repos/FlameGraph
rm out.*
sudo rm perf.data.*
sudo perf record -F 10000 -a -g -- ./sir_simulation
sudo perf script > out.perf
$FLAMEGRAPH_REPO/stackcollapse-perf.pl out.perf > out.folded
$FLAMEGRAPH_REPO/flamegraph.pl out.folded > flamegraph.svg
$FLAMEGRAPH_REPO/flamegraph.pl --flamechart out.folded > flamechart.svg
