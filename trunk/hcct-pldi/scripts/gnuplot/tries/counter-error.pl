# include
require "../config.pl";
require "../define-traces.pl";

print "set terminal x11\n";
print "set title \"max / average counter error among hot elements (% of the true frequency)\"\n";
print "set auto x\n";
print "set logscale y\n";
print "set xlabel \"benchmarks\"\n";
print "set ylabel \"max / avg error (%)\"\n";
print "set yrange [0.01:100]\n";
print "set style data histogram\n";
print "set xtic rotate by -45 scale 0\n";
print "set style histogram cluster gap 1\n";
print "set style fill solid border -2\n";
print "set boxwidth 0.8\n";

print("plot \\\n");

$TITLE_MAX_ERROR = "title 'max error'";
$TITLE_AVG_ERROR = "title 'avg error'";
for ($i = 0; $i < scalar(@TRACES); $i += 2) {
    print "'../../logs/lss-hcct-metrics-".$TRACES[$i+1].".log' using (\$20):xticlabels(1) ".$TITLE_MAX_ERROR.", \\\n".
          "'../../logs/lss-hcct-metrics-".$TRACES[$i+1].".log' using (\$21):xticlabels(1) ".$TITLE_AVG_ERROR;
    if ($i < scalar(@TRACES)-2) { print ", \\\n"; }
    $TITLE_MAX_ERROR = $TITLE_AVG_ERROR = "";
}

print "\n";
print "set terminal postscript eps color enhanced dashed \"Helvetica\" 16\n";
print "set output 'counter-error.eps'\n";
print "replot\n";
