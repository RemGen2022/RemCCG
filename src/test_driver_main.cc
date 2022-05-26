#include "proto_to_c.h"
#include "./build/c_proto.pb.h"
#include "src/mutator.h"
#include "src/text_format.h"
#include <ostream>
#include <sstream>
#include "src/libfuzzer/libfuzzer_macro.h"
#include "port/protobuf.h"
#include <algorithm>
#include <stdlib.h>
#include <fstream>
#include <sys/stat.h> //used for get file size
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/syscall.h>

//From ccg
//#include "ccg/ccg.h"
static unsigned getseed(void)
{
    unsigned seed;
    int urandom = open("/dev/urandom", O_RDONLY);

    if(urandom == -1)
        die("couldn't open /dev/urandom for reading.");

    read(urandom, &seed, sizeof(unsigned));
    close(urandom);

    return seed;
}


#include <assert.h>
#include <fcntl.h>
#include <error.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <vector>
// Testing path
std::string gcc_path("xxx"); // need to change
std::string llvm_path("xxx"); // need


std::string compiled_compiler = gcc_path;

// Flags
static bool gcc = true;
static bool clang = false;

static unsigned bound = 10;

static int timeout = 900;

std::string extra_option_for_gcc("-w");
std::string extra_option_for_clang("-w");

bool cmp(std::pair<std::vector<unsigned long long>, int> a, std::pair<std::vector<unsigned long long>, int> b){
    return a.second > b.second;
}

using namespace clang_fuzzer;

unsigned seed_ccg;
extern CommandlineOpt cmdline;
//extern std::vector<int> retVec;
class MyProtobufMutator : public protobuf_mutator::Mutator{
    public:
        void ProgramMutator(MainFunction* func, long long int seed_lpm);
};

void MyProtobufMutator::ProgramMutator(MainFunction* func, long long int seed_lpm){
    MyProtobufMutator myProtobufMutator;
    myProtobufMutator.Seed(seed_lpm);
    myProtobufMutator.Mutate(func, 10000);
}

pid_t waitpid_eintr(int status) {
  pid_t pid = 0;
  while ( (pid = waitpid(WAIT_ANY, &status, 0)) == -1 ) {
    if (errno == EINTR) {
      continue;
    } else {
      perror("waitpid");
      abort();
    }
  }
  return pid;
}

void save_timeout_cases(unsigned long time, std::string opt, std::string name){
    char filename[PATH_MAX]; 
    snprintf(filename, sizeof(filename), "case-timeout-%s-%lu.c", opt.c_str(), time);
	
    int source = open(name.c_str(), O_RDONLY, 0);
    int dest = open(filename, O_WRONLY | O_CREAT, 0644);

    // struct required, rationale: function stat() exists also
    struct stat stat_source;
    fstat(source, &stat_source);

    sendfile(dest, source, 0, stat_source.st_size);

    close(source);
}

void insert_sort(std::vector<testcase> pq){
    int size = pq.size();
    for (int j = 1; j < size; j++){
        testcase key = pq[j];
        int i = j -1;
        while (i >= 0 && key.coverage < pq[i].coverage){
            pq[i+1] = pq[i];
            i--;
        }
        pq[i+1] = key;
    }
}

struct testcase{
    unsigned seed;
    std::vector<int> grammar;
};


int main(int argc, char* argv[]){
    std::string generator;
    int bound;
   
    if (argc == 1 || argc > 3){
        printf("Please check the command line!\n");
        printf("    ./remgcc 1 10\n");
        printf("    ./remgcc 0 csmith\n");
        exit(0);
    } else{
        if (argc == 3){
            coverage_guided = atoi(argv[1]);
            bound = atoi(argv[2]);
        }
        if (argc == 3){
            coverage_guided = atoi(argv[1]);
            generator = argv[2];
        }
    }


    srand((unsigned)time(NULL));
    // From Prog-fuzz
    re = std::default_random_engine(r());

    int devnull = open("/dev/null", O_RDWR);
    if (devnull == -1)
	error(EXIT_FAILURE, errno, "/dev/null: open()");

    FILE *devnullf = fdopen(devnull, "r+");
    if (!devnullf)
	error(EXIT_FAILURE, errno, "/dev/null: fdopen()");

    struct timeval tv_start;
    if (gettimeofday(&tv_start, 0) == -1)
	error(EXIT_FAILURE, errno, "gettimeofday()");

    static char stderr_filename[PATH_MAX];
    snprintf(stderr_filename, sizeof(stderr_filename), "stderr-%lu.txt", tv_start.tv_sec);

    std::vector<testcase> pq;
    unsigned int nr_execs = 0;
    unsigned int nr_execs_without_new_bits = 0;

    // record file size
    static int file_size;
    static unsigned seed_ccg;
    static std::vector<int> seed_grammar;
    static std::vector<int> tempVecP;
    static unsigned int new_bits = 0;
    static int seed_execs = 0;
    static int num_seed = 0;
    static int stop = 0;
    
    while (true) {
        MyProtobufMutator myProtobufMutator;
        
        std::vector <unsigned long> grammar;
        for (auto i : pq){
            grammar.push_back(SquareSum(i.grammar));
        }
        int max_index = std::max_element(grammar.begin(), grammar.end()) - grammar.begin();
        auto it_selected = pq[max_index]; //this is random strategy
        seed_ccg = it_selected.seed;
        seed_grammar = it_selected.grammar;
       
	struct timeval tv;
	if (gettimeofday(&tv, 0) == -1)
	    error(EXIT_FAILURE, errno, "gettimeofday()");
        long long start_time = tv.tv_sec;

        
	int pipefd[2];
	if (pipe2(pipefd, 0) == -1)
	    error(EXIT_FAILURE, errno, "pipe2()");

        selected_mutant_file = "0.c";

        // pipe on tempVecP
        int fd[2];
 	    int ret = pipe(fd);
        if (ret == -1){
            error (EXIT_FAILURE, errno, "pipe()");
            return -1;
        }

        pid_t child_no = fork();
        if (child_no == -1)
			error(EXIT_FAILURE, errno, "fork()");
        if (child_no == 0){
                int fd_no; 
                fd_no = open("0.c", O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if (fd_no < 0)
                    error(EXIT_FAILURE, errno, "fd in no coverage_guided");
		
                dup2(fd_no, STDOUT_FILENO);
                char seed[20];
                sprintf(seed, "%lu", seed_ccg);
                if ((generator == "ccg") || (generator == "csmith")){
                    if(execlp(generator.c_str(), generator.c_str(), "--seed", seed, NULL))
                        error (EXIT_FAILURE, errno, "execlp in no coverage_guided");
                }
                if (generator == "yarpgen"){
                    if(execlp(generator.c_str(), generator.c_str(),  "--std=c99", "-s", seed, NULL))
                        error (EXIT_FAILURE, errno, "execlp in no coverage_guided");
                }
                
                std::vector<float>::iterator biggest = std::max_element(std::begin(allVec), std::end(allVec));
                
                std::string max_filename = std::to_string(std::distance(std::begin(allVec), biggest) + 1) + ".c";
                int source = open(max_filename.c_str(), O_RDONLY, 0);
                int dest = open("0.c", O_WRONLY | O_CREAT | O_TRUNC, 0644);

                // struct required, rationale: function stat() exists also
                struct stat stat_source;
                fstat(source, &stat_source);

                sendfile(dest, source, 0, stat_source.st_size);

                close(source);
                close(dest);
		        
                tempVecP = allGrammar.at(std::distance(std::begin(allVec), biggest));
                allGrammar.clear();
                allVec.clear();
               
	  	close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
		error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);

                //pipe for tempVecP
                close(fd[0]);
                char tmp[12] = {0x0};
                for (int i = 0; i < tempVecP.size(); i++){
                    int value = tempVecP[i];
                    sprintf(tmp, "%d", value);
                    write(fd[1], tmp, sizeof(tmp));
                }

                exit(0);
                }
            }
    	close(fd[1]);
        std::vector<int> tempVecPP(10);
        for (int i = 0; i < tempVecPP.size(); i++){
            char tmp_f[12];
            read(fd[0], tmp_f, sizeof(tmp_f));
            tempVecPP[i] = atoi(tmp_f);
        }
        close(fd[0]); // clear pipe file

        int status;
	while (true) {
		pid_t kid = waitpid(child_no, &status, 0);
		if (kid == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
				error(EXIT_FAILURE, errno, "waitpid()");
			}

		if (kid != child_no)
			error(EXIT_FAILURE, 0, "kid != child");

		if (WIFEXITED(status) || WIFSIGNALED(status))
			break;
        }


        // Set timer for each child
        // Deal with -O0
        pid_t timer1_pid = fork();
        if (timer1_pid == -1) error(EXIT_FAILURE, errno, "fork timer1 failed");
        if (timer1_pid == 0){
            sleep(timeout);
            exit(0);
        }
        //change the source file if test yarpgen
        if (generator == "yarpgen"){
            selected_mutant_file = "func.c";
        }

	pid_t p1 = fork();
        if (p1 == 0){
        	//sleep(1);
            	//fprintf(stderr, "This is the first child -O0\n");
	    	close(pipefd[1]);
	    	dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
			error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);
           
            // Test GCC
            if (gcc){
                if (execlp(compiled_compiler.c_str(), "cc1","-w", "-quiet", "-g", "-O0",extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
                //if (execlp(compiled_compiler.c_str(), "tcc","-w", "-g" "-std=c11", extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            // Test Clang
            if (clang){
		if (execlp(compiled_compiler.c_str(),"clang", "-w", "-g", "-O0",extra_option_for_clang.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				error(EXIT_FAILURE, errno, "execvp()");
            }
            return 0;
        }
        int status1 = 0;
        pid_t finished_first1 = waitpid_eintr(status1);
        if (finished_first1 == timer1_pid){
	    save_timeout_cases(nr_execs, "O0", selected_mutant_file);
            kill(p1, SIGKILL);
        }else{
            //printf("good on -O0\n");
            kill(timer1_pid, SIGKILL);
        }
        waitpid_eintr(status1);

        // Deal with -O1
        pid_t timer2_pid = fork();
        if (timer2_pid == -1) error(EXIT_FAILURE, errno, "fork timer2 failed");
        if (timer2_pid == 0){
            sleep(timeout);
            exit(0);
        }

        pid_t p2 = fork();
        if (p2 == 0){
            //sleep(1);
            //fprintf(stderr, "This is the second child -O1\n");
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
			error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);
            
            // Test GCC
            if (gcc){
                if (execlp(compiled_compiler.c_str(), "cc1","-w", "-quiet", "-g", "-O1",extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
                //if (execlp(compiled_compiler.c_str(), "tcc","-w", "-g" "-std=c99", extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            // Test Clang
            if (clang){
		 if (execlp(compiled_compiler.c_str(),"clang", "-w", "-g", "-O1",extra_option_for_clang.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            return 0;
        }
        int status2 = 0;
        pid_t finished_first2 = waitpid_eintr(status2);
        if (finished_first2 == timer2_pid){
	    save_timeout_cases(nr_execs, "O1", selected_mutant_file);
            kill(p2, SIGKILL);
        }else{
            //printf("good on -O1\n");
            kill(timer2_pid, SIGKILL);
        }
        waitpid_eintr(status2);


        // Deal with -O2
        pid_t timer3_pid = fork();
        if (timer3_pid == -1) error(EXIT_FAILURE, errno, "fork timer3 failed");
        if (timer3_pid == 0){
            sleep(timeout);
            exit(0);
        }
	pid_t p3 = fork();
        if (p3 == 0){
            //sleep(1);
            //fprintf(stderr, "This is the third child -O2\n");

		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
			error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);
           
            // Test GCC
            if (gcc){
                if (execlp(compiled_compiler.c_str(), "cc1","-w", "-quiet", "-g", "-O2", extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            // Test Clang
            if (clang){
		 if (execlp(compiled_compiler.c_str(),"clang", "-w", "-g", "-O2", extra_option_for_clang.c_str(), selected_mutant_file.c_str(), NULL) == -1)
			error(EXIT_FAILURE, errno, "execvp()");
            }
            return 0;
        }
        int status3 = 0;
        pid_t finished_first3 = waitpid_eintr(status3);
        if (finished_first3 == timer3_pid){
 	    save_timeout_cases(nr_execs, "O2", selected_mutant_file);
            kill(p3, SIGKILL);
        }else{
            //printf("good on -O2\n");
            kill(timer3_pid, SIGKILL);
        }
        waitpid_eintr(status3);

        // Deal with -O3
        pid_t timer4_pid = fork();
        if (timer4_pid == -1) error(EXIT_FAILURE, errno, "fork timer4 failed");
        if (timer4_pid == 0){
            sleep(timeout);
            exit(0);
        }

	    pid_t p4 = fork();
        if (p4 == 0){
            //sleep(1);
            //fprintf(stderr, "This is the 4th child -O3\n");

		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
			error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);
          
            // Test GCC
            if (gcc){
                if (execlp(compiled_compiler.c_str(), "cc1","-w", "-quiet", "-g", "-O3", extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            // Test Clang
            if (clang){
		if (execlp(compiled_compiler.c_str(),"clang", "-w", "-g", "-O3", extra_option_for_clang.c_str(), selected_mutant_file.c_str(), NULL) == -1)
		    error(EXIT_FAILURE, errno, "execvp()");
            }

            return 0;
        }
        int status4 = 0;
        pid_t finished_first4 = waitpid_eintr(status4);
        if (finished_first4 == timer4_pid){
	    save_timeout_cases(nr_execs, "O3", selected_mutant_file);
            kill(p4, SIGKILL);
        }else{
            //printf("good on -O3\n");
            kill(timer4_pid, SIGKILL);
        }
        waitpid_eintr(status4);

        // Deal with -Os
        pid_t timer5_pid = fork();
        if (timer5_pid == -1) error(EXIT_FAILURE, errno, "fork timer5 failed");
        if (timer5_pid == 0){
            sleep(timeout);
            exit(0);
        }

	    pid_t p5 = fork();
        if (p5 == 0){
            //sleep(1);
            //fprintf(stderr, "This is the 5th child -Os\n");

		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		dup2(devnull, STDOUT_FILENO);

		int stderr_fd = open(stderr_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (stderr_fd == -1)
			error(EXIT_FAILURE, errno, "open()");

		dup2(stderr_fd, STDERR_FILENO);
            
            // Test GCC
            if (gcc){
                if (execlp(compiled_compiler.c_str(), "cc1","-w", "-quiet", "-g", "-Os", extra_option_for_gcc.c_str(), selected_mutant_file.c_str(), NULL) == -1)
				    error(EXIT_FAILURE, errno, "execvp()");
            }
            // Test Clang
            if (clang){
		if (execlp(compiled_compiler.c_str(),"clang", "-w", "-g", "-Os", extra_option_for_clang.c_str(), selected_mutant_file.c_str(), NULL) == -1)
		    error(EXIT_FAILURE, errno, "execvp()");
            }

            return 0;
        }
        int status5 = 0;
        pid_t finished_first5 = waitpid_eintr(status5);
        if (finished_first5 == timer5_pid){
            //printf("time out on -Os\n");
            save_timeout_cases(nr_execs, "Os", selected_mutant_file);
            kill(p5, SIGKILL);
        }else{
            //printf("good on -Os\n");
            kill(timer5_pid, SIGKILL);
        }
        waitpid_eintr(status5);


	close(pipefd[0]);
	FILE *f = fdopen(pipefd[1], "w");
	if (!f)
		error(EXIT_FAILURE, errno, "fdopen()");
	fclose(f);

        
	++nr_execs;
        ++seed_execs;
	//Check for crash or timeout
	{
		FILE *f = fopen(stderr_filename, "r");
		if (!f)
			error(EXIT_FAILURE, errno, "fopen()");

		static char buffer[10 * 4096 * 1024];
		size_t len = fread(buffer, 1, sizeof(buffer), f);
		fclose(f);

		if (len > 0) {
			buffer[len - 1] = '\0';
				
                if (gcc){
                	if ((strstr(buffer, "internal") || strstr(buffer, " fault") || strstr(buffer, "fatal")) || strstr(buffer, "internal")) {
		    		char filename[PATH_MAX];
		    		snprintf(filename, sizeof(filename), "case-crash-%lu.c", nr_execs);
                  
                    		int source = open(selected_mutant_file.c_str(), O_RDONLY, 0);
                    		int dest = open(filename, O_WRONLY | O_CREAT, 0644);

                    		// struct required, rationale: function stat() exists also
                    		struct stat stat_source;
                    		fstat(source, &stat_source);

                    		sendfile(dest, source, 0, stat_source.st_size);

                    		close(source);
                    		close(dest);

                    		fclose(fopen(stderr_filename, "w"));
                    	}
		 }
		 if (clang ){
                	if ((strstr(buffer, "Assertion") || strstr(buffer, "Please"))){
                    		char filename[PATH_MAX];
				snprintf(filename, sizeof(filename), "case-crash-%lu.c", nr_execs);

                    		FILE* clang_output = fopen("clang-output.txt", "a+");
                    		fputs(buffer, clang_output);
                    		fclose(clang_output);
                  
                    		fclose(fopen(stderr_filename, "w"));
                    	}
		    }
		}
	    }	
	}
    return 0;
}



