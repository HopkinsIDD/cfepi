$FILE = $ARGV[0];
$TFILE = $ARGV[1];
$OFILE = $ARGV[2];
print "in: $FILE\ntmp: $TFILE\nout: $OFILE\n";

`od -i $FILE > $TFILE`;

open(IN,"< $TFILE") or die "COULD NOT OPEN $TFILE $!\n";
open(OUT,"> $OFILE") or die "COULD NOT OPEN $OFILE $!\n";

$counter = 0;
while($line = <IN>){
  chop $line;
  $line =~ s/^\d*\s*//;
  @line = split(/ +/,$line);
  for $word (@line){
    ++$counter;
    print OUT $word;
    print OUT "\t";
    if($counter >= 5){
      print OUT "\n";
      $counter = 0;
    }
  }
}
