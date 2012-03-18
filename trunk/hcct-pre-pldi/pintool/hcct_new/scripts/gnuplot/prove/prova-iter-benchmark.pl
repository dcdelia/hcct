# include
require "../config.pl";
require "../define-traces.pl";

print "set terminal x11\n";
print "set title \"avg/max frequency of contexts not included in HCCT (% of the hottest context)\"\n";
print "set auto x\n";
print "set key under\n";
print "set logscale y\n";
print "set xlabel \"benchmarks\"\n";
print "set ylabel \"avg/max frequency (%)\"\n";
print "set yrange [0.00001:*]\n";
print "set style data histogram\n";
#print "set style histogram rows\n";
print "set style histogram cluster gap 1\n";
print "set xtic rotate by -45 scale 0\n";
print "set style fill solid border -2\n";
print "set boxwidth 0.8\n";

print("plot \\\n");

print("    newhistogram \"average\"\\\n");

$TITLE_LSS = "title 'lss'";
$TITLE_LC = "title 'lc'";
for ($i = 0; $i < scalar(@TRACES); $i += 2) {
    print "        '../../logs/lss-hcct-metrics-".$TRACES[$i+1].".log' using (\$11):xticlabels(1) ".$TITLE_LSS.", \\\n".
          "        '../../logs/lc-hcct-metrics-".$TRACES[$i+1].".log' using (\$11) ".$TITLE_LC.", \\\n";
    $TITLE_LSS = $TITLE_LC = "";
}

#print("    newhistogram \"maximum\"\\\n");

#$TITLE_LSS = "title 'lss'";
#$TITLE_LC = "title 'lc'";
#for ($i = 0; $i < scalar(@TRACES); $i += 2) {
    #print "        '../../logs/lss-hcct-metrics-".$TRACES[$i+1].".log' using (\$13):xticlabels(1) ".$TITLE_LSS.", \\\n".
          #"        '../../logs/lc-hcct-metrics-".$TRACES[$i+1].".log' using (\$13) ".$TITLE_LC;
    #if ($i < scalar(@TRACES)-2) { print ", \\\n"; }
    #$TITLE_LSS = $TITLE_LC = "";
#}

print "\n";
print "set terminal postscript eps color enhanced dashed \"Helvetica\" 16\n";
print "set output 'avg-max-uncovered.eps'\n";
print "replot\n";
