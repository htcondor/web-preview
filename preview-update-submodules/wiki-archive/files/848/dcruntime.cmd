@rem ='these 3 lines bootstrap from windows cmd .bat into perl
@perl.exe %~sf0 %*
@goto :EOF ';
# perl begins here
#
use strict;
use warnings;

if (@ARGV == 0) {
   print "Useage: $0 [total][probe][recent] [<filename> | <machine> [<verbosity>][<arguments>]]\n";
   print "    total       : Show columns for total runtime (default)\n";
   print "    probe       : Show columns for Max,Min & Std\n";
   print "    recent      : Show columns for Recent runtime. If used alone show only Recent runtime.\n";
   print "    <filename>  : open a file containing a long-form classad\n";
   print "                  and parse it for runtime statistics\n";
   print "    <machine>   : call condor_status -long -direct for the given machine\n";
   print "                  and parse the output for runtime statistics\n";
   print "    <verbosity> : verbosity argument for -statistics, e.g ALL:2\n";
   print "    <arguments> : arguments for condor_status\n";
   exit 1;
}

my $machine = "$ENV{USERDOMAIN}.cs.wisc.edu";
my $verb    = "";
my $classad_file = "";
my $status_opts = "";
my $not_file = 0;
my $recent = 0;
my $totals = 0;   # user explicitly chose to show runtime totals.
my $base   = 1;   # implicitly showing runtime totals
my $probe  = 0;
my %ad=();        # all classad attributes
my %rt=();        # attributes ending in Runtime, but not starting with Recent, with "Runtime$" removed
my %recent_rt=(); # all attributes starting with Recent and ending with Runtime, both "^Recent" and "Runtime$" removed
my %ema=();       # all attributes that have Exponential moving average values
my %probe=();     # base keys for all attributes of type Probe (i.e. have min,max,avg,std)
my %key_map = (
  "DCPipe" => "DCPipeMessages",   # "RecentDCPipe" => "RecentDCPipeMessages",
  "DCSignal" => "DCSignals",      # "RecentDCSignal" => "RecentDCSignals",
  "DCSocket" => "DCSockMessages", # "RecentDCSocket" => "RecentDCSockMessages",
  "DCTimer" => "DCTimersFired",   # "RecentDCTimer" => "RecentDCTimersFired",
  "DCCommand" => "DCCommands",    # "RecentDCCommand" => "RecentDCCommands",
);

# parse arguments
#
foreach (@ARGV) {
    if ($_ =~ /^\-/) {
       $not_file = 1;
    }

    #pick up bare verbosity flags
    if ($_ =~ /^(DC|DaemonCore|ALL|Scheduler|SCHEDD|Negotiator|Collector|STARTD|MASTER)\:[0-3ZRDL!]+/i) {
       if ($verb eq "") {
          $verb = $_;
       } else {
          $verb .= ",$_";
       }
       next;
    }
    #pick up totals option
    if ($_ =~ /^total[s]?$/i) {
       $totals = 1;
       $base = 1;
       next;
    }

    #pick up recent option
    if ($_ =~ /^recent[=:]([01])$/) {
       $recent = $1;
       # print "recent = $recent";
       next;
    }
    if ($_ =~ /^recent$/i) {
       $recent = 1;
       $base = $probe || $totals;
       next;
    }
    #pick up probe option
    if ($_ =~ /^probe$/i) {
       $probe = 1; $base = 1;
       # print "probe = $probe\n";
       next;
    }
    if ($_ =~ /^Miron$/i) {
       $probe = 1; $base = 1;
       # print "Miron = $probe\n";
       next;
    }

    if ($not_file) {
       $status_opts = $status_opts . " " . $_;
       next;
    }

    if ($_ =~ /\\/) {
       $classad_file = $_;
       next;
    }
    if (-f $_) {
       $classad_file = $_;
       next;
    }

    $status_opts = $status_opts . " " . $_;
}

#print "classad_file=$classad_file\n";
#print "status_opts=$status_opts\n";

if ($classad_file) {
   ParseClassAdFromFile($classad_file, \%ad, \%rt, \%recent_rt, \%ema, \%probe);
} else {
   if ($status_opts =~ /^ *\-startd *$/i) {
      $status_opts .= " slot1\@$machine -direct $machine";
   }
   if ($status_opts =~ /^ *\-schedd *$/i) {
      $status_opts .= " -direct $machine";
   }
   if (($status_opts =~ /\-direct/i) && !($status_opts =~ /\-statistics/i)) {
      $status_opts .= " -statistics";
      if ($verb eq "") {
         $verb = "DC:2";
      }
      $status_opts .= " $verb";
   }
   if ($status_opts eq "" && !($verb eq "")) {
      my $daemon = "-schedd";
      if ($verb =~ /(Scheduler|SCHEDD)/i) {
         $daemon = "-schedd";
      } else {
         if ($verb =~ /STARTD/i) {
            $daemon = "-startd slot1\@$machine";
         }
      }
      $status_opts = "-direct $machine $daemon -statistics $verb";
   }
   ParseClassAdFromStatus($status_opts, \%ad, \%rt, \%recent_rt, \%ema, \%probe);
}

# dump the %ad
#
# foreach my $key (sort keys %ad) { print "$ad{$key} = $key\n"; }

# dump the probe set
#
#if ($probe) {
#   print "probes = \n";
#   foreach my $key (sort keys %probe) { print "$key = $probe{$key}\n"; }
#   print "\n";
#}

# dump the runtimes
#
print "\n";          #12345678  123456 /1234567890
if ($base)   { print " Average   Total     Runtime"; }
if ($probe)  { print "      Max      Min    Std "; }
if ($recent) { print "RecntAvg  Recent  RecntRunTm"; }
print " Name\n\n";

foreach my $rtkey (sort keys %rt) {
   my $key = $rtkey;
   if (exists $key_map{$key}) { $key = $key_map{$key}; if ($base) {print "\n"; } }
   if ($base) {
      if (exists $ad{$key}) {
         PrintRuntime($ad{$key}, $ad{$rtkey."Runtime"});
      } elsif ($recent) {
         PrintRuntime("0","0.0");
      }
   }
   if ($probe) {
      if (exists $probe{$key}) {
         printf " %8.6f %8.6f %6.3f ", $ad{$key."RuntimeMax"}, $ad{$key."RuntimeMin"}, $ad{$key."RuntimeStd"};
      } else {
         printf " %8s %8s %6s ", "na", "na", "na";
      }
   }
   if ($recent) {
      if (exists $ad{"Recent".$key}) {
         PrintRuntime($ad{"Recent".$key}, $ad{"Recent".$rtkey."Runtime"});
      } elsif ($base) {
         print "      na      na /        na";
      } else {
         next;
      }
   }
   print " $key\n";
}

exit 0;

sub PrintRuntime
{
   my $count = shift;
   my $runtime = shift;
   my $per = 0.0;
   $per = $runtime/$count if $count != 0;

   # align runtimes by .
   my $ix = index($runtime, '.');
   while ($ix < 4) {
      $runtime = " $runtime";
      $ix += 1;
   }
   $runtime = substr $runtime, 0, 9;
   printf("%8.3f  %6s /%-10s", $per, $count, $runtime);
}

sub PrintRuntimeOld
{
   my $key = shift;

   if (exists $ad{$key}) {
      my $attr = $key . "Runtime";
      my $count   = $ad{$key};
      my $runtime = $ad{$attr};
      my $per     = 0.0;
      $per = $runtime/$count if $count != 0;

      my $ix = index($runtime, '.');
      while ($ix < 4) {
         $runtime = " $runtime";
         $ix += 1;
      }
      $runtime = substr $runtime, 0, 9;
      printf("%8.3f  %6s /%-10s %s\n", $per, $count, $runtime, $key);
   }
}

sub ParseClassAdFromStatus
{
   my $opts    = shift;
   my $hashref = shift;
   my $runtime = shift;
   my $recent  = shift;
   my $ema     = shift;
   my $probe   = shift;

  # print "ParseClassAd on file <$options>\n";

   my $stat   = "condor_status -long $opts";
   print "$stat\n";

   open(FH,"$stat |") || die "Can not open <$stat>: $!\n";
   while(<FH>) {
      # print $_;
      if($_ =~ /^(\w+)\s*=\s*(.*)\s*/) {
        # print "Option $1 = $2\n";
        # next if ( $1 eq "version" );
         ${$hashref}{$1} = $2;
         my $attr = $1;
         my $value = $2;
         if ($attr =~ /^Recent/) {
            $attr =~ s/^Recent//;
            if ($attr =~ /Runtime$/) {
               $attr =~ s/Runtime$//;
               ${$recent}{$attr} = $value;
            }
         } else {
            if ($attr =~ /Runtime$/) {
               $attr =~ s/Runtime$//;
               ${$runtime}{$attr} = $value;
            }
            elsif ($attr =~ /RuntimeAvg$/) {
               $attr =~ s/RuntimeAvg$//;
               ${$probe}{$attr} = $value;
            }
            elsif ($attr =~ /^.+_[0-9]+[hmsHMS]/) {
               ${$ema}{$attr} = $value;
            }
         }
      } else {
          #  print "no match: $_";
      }

   }
   close(FH);

  # ${$hashref}{"success"} = 1;

  # print "Leaving ParseOptions - success == ${$hashref}{\"success\"}\n" ;
   return 1;
}

sub ParseClassAdFromFile
{
   my $file = shift;
   my $hashref = shift;
   my $runtime = shift;
   my $recent  = shift;
   my $ema     = shift;
   my $probe   = shift;

  # print "ParseClassAdFromFile on file <$options>\n";

   if(!(-f $file)) {
      print "<$file> is not a valid filename\n";
      return 0;
   }

   print "from $file:\n";

   open(FH,"<$file") || die "Can not open <$file>: $!\n";
   while(<FH>) {
      # print $_;
      if($_ =~ /^(\w+)\s*=\s*(.*)\s*/) {
        # print "Option $1 = $2\n";
        # next if ( $1 eq "version" );
         ${$hashref}{$1} = $2;
         my $attr = $1;
         my $value = $2;
         if ($attr =~ /^Recent/) {
            $attr =~ s/^Recent//;
            if ($attr =~ /Runtime$/) {
               $attr =~ s/Runtime$//;
               ${$recent}{$attr} = $value;
            }
         } else {
            if ($attr =~ /Runtime$/) {
               $attr =~ s/Runtime$//;
               ${$runtime}{$attr} = $value;
            }
            elsif ($attr =~ /RuntimeAvg$/) {
               $attr =~ s/RuntimeAvg$//;
               ${$probe}{$attr} = $value;
            }
            elsif ($attr =~ /^.+_[0-9]+[hmsHMS]/) {
               ${$ema}{$attr} = $value;
            }
         }
      } else {
          #  print "no match: $_";
      }

   }
   close(FH);

  # print "Leaving ParseClassAdFromFile\n" ;
   return 1;
}
