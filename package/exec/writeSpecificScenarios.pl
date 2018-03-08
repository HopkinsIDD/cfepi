use strict;
use warnings;
use File::Find;
use Cwd 'abs_path';
my $path = abs_path($0);

## Put files in the src directory
my $outdir = 'src';
## Create all of the stubs for everything in existence so far
write_all(
  "no",
  [],
  [],
  [],
  []
);
write_all(
  "flat",
  ['step_t','float'],
  ['start_time','rate'],
  [],
  []
);

write_all(
  "single",
  [],
  [],
  ['step_t','float'],
  ['time','rate']
);
# write_all($name,\@c_types_beta,\@names_beta)
sub write_all {
  write_c_file(@_);
  write_h_file(@_);
}

# write_c_file($name,\@c_types_beta,\@names_beta)
sub write_c_file {
  my $name = $_[0];
  my @c_types_beta = @{$_[1]};
  my @names_beta = @{$_[2]};
  my @c_types_susceptible = @{$_[3]};
  my @names_susceptible = @{$_[4]};

  ## Check to see if file exists, then open for writing
  my $filename = "$outdir/${name}_intervention.c";
  if(-e $filename){
    print STDERR "Will not overwrite existing file\n";
    return;
  }
  print("-$filename-\n");
  open(C,"> $filename") or die "Could not open file $filename: $!\n";
  
  ## Start printing to file
  ## Include statements
  print C "#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n#include <R_ext/print.h>\n";
  print C "#include \"${name}_intervention.h\"\n";
  ## constructor for the beta params
  print C "param_beta_t param_${name}_beta(";
  my $counter = 0;
  for my $this_name_beta (@names_beta){
    my $c_type_beta = $c_types_beta[$counter];
    print C "$c_type_beta $this_name_beta";
    if($counter < $#names_beta){
      print C ", ";
    }
    ++$counter;
  }
  print C "){
  param_beta_t rc;
  strcpy(rc.type , \"$name\");
  rc.data = malloc(1 * sizeof(data_beta_${name}_t));\n";
  for my $this_name_beta (@names_beta){
    my $c_type_beta = $c_types_beta[$counter];
    print C "  (* ( (data_beta_${name}_t*) rc.data) ).$this_name_beta = $this_name_beta;\n";
    ++$counter;
  }
  print C "  return(rc);\n};\n\n";
  ## end constructor for the beta params
  ## destructor for the beta params
  print C "bool_t free_param_${name}_beta(param_beta_t rc){\n";
  $counter = 0;
  ## Check the type to make sure we aren't freeing the wrong thing
  print C "  if(strcmp(rc.type,\"$name\")!=0){\n    fprintf(stderr,\"Attempting to free a param_beta_t with the wrong destructor\\n\");\n    return(1);\n  }\n";
  for my $this_type (@c_types_beta){
    ## Make sure to free arrays
    ++$counter;
    if($this_type =~ /\*/){
      print C "  free((*rc.data).$names_beta[$counter]);\n";
    }
  }
  print C "  free(rc.data);\n  return(0);\n";
  print C "};\n\n";
  ## end destructor of the beta params
  print C "// Finish writing these\nbool_t ${name}_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars){\n  return(1);\n}\n\n";
  ## constructor for the susceptible params
  print C "param_susceptible_t param_${name}_susceptible(";
  $counter = 0;
  for my $this_name_susceptible (@names_susceptible){
    my $c_type_susceptible = $c_types_susceptible[$counter];
    print C "$c_type_susceptible $this_name_susceptible";
    if($counter < $#names_susceptible){
      print C ", ";
    }
    ++$counter;
  }
  print C "){
  param_susceptible_t rc;
  strcpy(rc.type,\"$name\");
  rc.data = malloc(1 * sizeof(data_susceptible_${name}_t));\n";
  for my $this_name_susceptible (@names_susceptible){
    my $c_type_susceptible = $c_types_susceptible[$counter];
    print C "  (* ( (data_susceptible_${name}_t *) rc.data) ).$this_name_susceptible = $this_name_susceptible;\n";
    ++$counter;
  }
  print C "  return(rc);\n};\n\n";
  ## end constructor for the susceptible params
  ## destructor for the susceptible params
  print C "bool_t free_param_${name}_susceptible(param_susceptible_t rc){\n";
  $counter = 0;
  ## Check the type to make sure we aren't freeing the wrong thing
  print C "  if(strcmp(rc.type,\"$name\")!=0){\n    fprintf(stderr,\"Attempting to free a param_susceptible_t with the wrong destructor\\n\");\n    return(1);\n  }\n";
  for my $this_type (@c_types_susceptible){
    ## Make sure to free arrays
    if($this_type =~ /\*/){
      print C "  free((*rc.data).$names_susceptible[$counter]);\n";
    }
    ++$counter;
  }
  print C "  free(rc.data);\n  return(0);\n";
  print C "};\n\n";
  ## end destructor of the susceptible params
  print C "// Finish writing these\nvoid ${name}_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars){\n  return;\n}\n\n";
}
# write_h_file($name,\@c_types_beta,\@names_beta)
sub write_h_file {
  my $name = $_[0];
  my @c_types_beta = @{$_[1]};
  my @names_beta = @{$_[2]};
  my @c_types_susceptible = @{$_[3]};
  my @names_susceptible = @{$_[4]};

  ## Check to see if file exists, then open for writing
  my $filename = "$outdir/${name}_intervention.h";
  print("-$filename-\n");
  open(H,"> $filename") or die "Could not open file $filename: $!\n";
  
  ## Start printing to file
  ## preprocessor headers
  my $NAME = uc $name;
  print H "#ifndef ${NAME}_INTERVENTION_H_\n";
  print H "#define ${NAME}_INTERVENTION_H_\n";
  ## Include statements
  print H "#include <stdlib.h>\n#include <stdio.h>\n";
  print H "#include \"twofunctions.h\"\n";
  ## constructor for the beta params
  print H "typedef struct {\n";
  my $counter = 0;
  for my $this_name_beta (@names_beta){
    my $c_type_beta = $c_types_beta[$counter];
    print H "  $c_type_beta $this_name_beta;\n";
    ++$counter;
  }
  print H "} data_beta_${name}_t;\n\n";
  print H "param_beta_t param_${name}_beta(";
  for my $this_type (@c_types_beta){
    print H "$this_type";
    if($this_type ne $c_types_beta[$#c_types_beta]){
      print H ", ";
    };
  }
  print H ");\n";
  ## end constructor for the beta params
  ## destructor for the beta params
  print H "bool_t free_param_${name}_beta(param_beta_t);\n";
  ## end destructor of the susceptible params
  ## constructor for the susceptible params
  print H "typedef struct {\n";
  $counter = 0;
  for my $this_name_susceptible (@names_susceptible){
    my $c_type_susceptible = $c_types_susceptible[$counter];
    print H "  $c_type_susceptible $this_name_susceptible;\n";
    ++$counter;
  }
  print H "} data_susceptible_${name}_t;\n\n";
  print H "param_susceptible_t param_${name}_susceptible(";
  for my $this_type (@c_types_susceptible){
    print H "$this_type ";
    if($this_type ne $c_types_susceptible[$#c_types_susceptible]){
      print H ", ";
    };
  }
  print H ");\n";
  ## end constructor for the params
  ## destructor for the params
  print H "bool_t free_param_${name}_susceptible(param_susceptible_t);\n";
  ## end destructor of the susceptible params
  ## beta_t
  print H "bool_t ${name}_beta(step_t time, person_t person1, person_t person2, var_t var1, var_t var2,param_beta_t pars);\n";
  ## end beta_t
  ## susceptible_t
  print H "void ${name}_susceptible(var_t** states, step_t time, step_t ntime, person_t npop, param_susceptible_t pars);\n";
  ## end susceptible_t
  ## close preprocessor directive
  print H "#endif //${NAME}_INTERVENTION_H_\n";
}

