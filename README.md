# Linker
Go to terminal and type:

$ make

This will create linker.exe, then you can test inputs:


$ ./linker "input-file"

The folder lab1samples also has scripts to check all the sample inputs. To test them:


Create yourself a directory <your-outdir> where your outputs will be created.
  
$ cd lab1samples


$ ./runit.sh "your-outdir" "path-to-linker.exe"
  
  
$ ./gradeit.sh "." "your-outdir" 
