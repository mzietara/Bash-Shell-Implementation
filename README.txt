This is an implementation of my own mini-shell. The shell launches commands in new processes, as well as supports the built-in commands cd and exit. The shell will also support standard input, output and error redirection, as well as the pipe (|) operator to chain commands. NOTE: This shell only works on linux systems to do certain dependencies not found on other systems.

Sample shell commands:

$ ./shell
/home/bogdan/csc209/a3/> pwd
/home/bogdan/csc209/a3/> ls -l
/home/bogdan/csc209/a3/> ls -l > file.out
/home/bogdan/csc209/a3/> cat file.out
/home/bogdan/csc209/a3/> ls -l | wc -l
/home/bogdan/csc209/a3/> ls -l | grep file1
/home/bogdan/csc209/a3/> ls -l > filelist.out | wc -l
/home/bogdan/csc209/a3/> cat filelist.out
/home/bogdan/csc209/a3/> ls -l | wc -l > wc.out
/home/bogdan/csc209/a3/> cat wc.out
/home/bogdan/csc209/a3/> ls -l > ls.out | wc -l > wc.out
/home/bogdan/csc209/a3/> cat ls.out
/home/bogdan/csc209/a3/> cat wc.out
/home/bogdan/csc209/a3/> rm -f ls.out wc.out
/home/bogdan/csc209/a3/> ls -l | grep bogdan | wc -l
/home/bogdan/csc209/a3/> ps aux | grep bogdan | grep bash | grep -v grep | wc -l