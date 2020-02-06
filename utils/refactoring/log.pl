use strict;
use warnings;

use File::Find;
use File::Basename;
#use Text::Balanced qw{extract_delimited, extract_balanced};

my %levels = (error => 1, info => 200, none => 0, warn => 100);
my $basedir = "u:/src/megamol";

my @exts = qw(.cpp .hpp .h);
my @ignoredirs = ("\/.git\/", "${basedir}/external/", "${basedir}/include/", "${basedir}/build\\S*/");

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
                my %args_to_alias = ();
                my %alias_to_args = ();
                
                my $any_hit = $file_content =~ /DefaultLog/s;
                if (not $any_hit == 1) {
                    return;
                }
                
                # get rid of all parentheses with potential recursion inside
                # and replace contents by alias
                my $parenthesis_cleanup = qr/(\((?:[^()]++|(?R))*\))/s;
                my $curr_args_entry = "a";
                while ($file_content =~ /$parenthesis_cleanup/g) {
                    #print $1 . " = " . $curr_args_entry . "\n";
                    $args_to_alias{$1} = $curr_args_entry;
                    $alias_to_args{$curr_args_entry} = $1;
                    $curr_args_entry++;
                }
                my $clean_file_content = $file_content;
                $clean_file_content =~ s/$parenthesis_cleanup/(${args_to_alias{$1}})/g;
                
                my @matches = $clean_file_content =~ /[a-zA-Z:.]*DefaultLog\S+\(\w+\)/gs;
                if ($#matches > -1) {
                    #print "\n" . "-=" x 30 . "\n";
                    #print $File::Find::name . "\n";
                    foreach my $m (@matches) {
                        #print "-" x 50 . "\n";
                        #print "$m\n";
                        if ($m =~ /DefaultLog.Write(\S+)\((\w+)\)/xs) {
                            my $level = $1;
                            my $alias = $2;
                            $alias_to_args{$alias} =~ /^\((.*?)\)$/s;
                            my $args = $1;
                            #print "found candidate: ";
                            if ($level eq "Msg") {
                                # split off the errorlevel
                                $args =~ /^([^,]+),\s*(.*?)$/s;
                                $level = $1;
                                $args = $2;
                            }
                            $level = cleanup($level);
                            $args = cleanup($args);
                            
                            # find any suffixes to level constants and recover them
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
                                print "weird level $level in $name$ext\n";
                                next;
                            }
                            
                            # try to find messages that have formatted arguments
                            my $format = ($args =~ /^(.*?")((?:,[^,]+)*)$/);
                            if ($format == 1 and $2 ne "") {
                                #print "format $1 rest $2";
                                my $fmt = $1;
                                my $rest = $2;
                                my $havereplace = ($fmt =~ s/%([0-9.]*)l?[sduif]/{}/g);
                                if ($havereplace == 1 and $1 ne "") {
                                    print "format $fmt not understood in $name$ext\n";
                                    next;
                                }
                                $args = $fmt . $rest;
                            }
                            #print "=> level $reallevel\n=> args $args\n";
                            
                            #recover original text
                            my $orig = $m;
                            $orig =~ s/\Q($alias)/$alias_to_args{$alias}/s;
                            push @from, $orig;
                            # generate new command
                            push @to, "hurzofant($reallevel, $args)"
                        }
                    }
                    # do the replacing
                    #print "\n\n";
                    for (my $x = 0; $x <= $#from; $x++) {
                        $file_content =~ s/\Q$from[$x]/$to[$x]/s;
                    }
                    #print $file_content;
                    
                    # TODO: replace file
                    #exit 0;
                }
            }
        }
    }
}

find({ wanted => \&wanted, no_chdir => 0}, $basedir);