use strict;
use warnings;

use File::Find;
use File::Basename;
#use Text::Balanced qw{extract_delimited, extract_balanced};

my %levels = (error => 1, info => 200, none => 0, warn => 100);
my $basedir = "u:/src/megamol";

my @exts = qw(.cpp .hpp .h);
my @ignoredirs = ("\/.git\/", "${basedir}/external/", "${basedir}/build\\S*/");

sub checkdir {
    my $dir = shift;
    my $found = 0;
    foreach my $i (@ignoredirs) {
        if ($dir =~ /$i/) {
            $found = 1;
            last;
        }
    }
    return not $found;   
}

sub cleanup {
    my $stuff = shift;
    $stuff =~ s/^\s*(.*?)\s*$/$1/gs;
    return $stuff;
}

sub wanted {
    if (not -d) {
        if (checkdir($File::Find::dir)) {
            my ($name, $dir, $ext) = fileparse($_, @exts);
            if ($ext) {
                my $file_content = do{local(@ARGV,$/)=$File::Find::name;<>};
                my @from = ();
                my @to = ();
                
                my @matches = $file_content =~ /[a-zA-Z:.]*DefaultLog\S+\((?:[^()]++|(?R))*\);/xgs;
                if ($#matches > -1) {
                    print "\n" . "-=" x 30 . "\n";
                    print $File::Find::name . "\n";
                    foreach my $m (@matches) {
                        print "-" x 50 . "\n";
                        print "$m\n";
                        if ($m =~ /DefaultLog.Write(\S+)\(((?:[^()]++|(?R))*)\)/xs) {
                            my $level = $1;
                            my $args = $2;
                            print "found candidate: ";
                            if ($level eq "Msg") {
                                $args =~ /^([^,]+),\s*(.*?)$/s;
                                $level = $1;
                                $args = $2;
                            }
                            $level = cleanup($level);
                            $args = cleanup($args);
                            my $explicit = ($level =~ /\S*LEVEL_(\w+)(?:\s*(.*?))?$/s);
                            my $reallevel;
                            if ($explicit == 1) {
                                #print "----> this does not have a clean level\n";
                                $reallevel = $levels{lc($1)};
                                if (defined $2) {
                                    $reallevel .= " $2";
                                }
                            } else {
                                if ($level =~ /^\d+$/) {
                                    $reallevel = $level;    
                                } else {
                                    $reallevel = $levels{lc($level)};
                                }
                            }
                            if (not defined $reallevel) {
                                print "WTF\n";
                            }
                            my $format = ($args =~ /^(.*?")((?:,[^,]+)*)$/);
                            if ($format == 1 and $2 ne "") {
                                #print "format $1 rest $2";
                                my $fmt = $1;
                                my $rest = $2;
                                my $havereplace = ($fmt =~ s/%([0-9.]*)l?[sduif]/{}/g);
                                if ($havereplace == 1 and $1 ne "") {
                                    print "WTF2\n";
                                }
                                $args = $fmt . $rest;
                            }
                            #print "=> level $reallevel\n=> args $args\n";
                            # TODO: generate new command
                            push @from, $m;
                            push @to, "hurzofant($reallevel, $args);"
                        }
                    }
                    # do the replacing
                    print "\n\n";
                    for (my $x = 0; $x <= $#from; $x++) {
                        $file_content =~ s/\Q$from[$x]/$to[$x]/s;
                    }
                    print $file_content;
                    exit 0;
                }
            }
        }
    }
}

find({ wanted => \&wanted, no_chdir => 0}, $basedir);