/************************************************************************

 test.c

 These programs are designed to test the OS502 functionality

 Read Appendix B about test programs and Appendix C concerning
 system calls when attempting to understand these programs.

 ************************************************************************/

#define          USER
#include         "global.h"
#include         "protos.h"
#include         "syscalls.h"

#include         "stdio.h"
#include         "string.h"
#include         "stdlib.h"
#include         "math.h"

INT16 Z502_PROGRAM_COUNTER;

extern long Z502_REG1;
extern long Z502_REG2;
extern long Z502_REG3;
extern long Z502_REG4;
extern long Z502_REG5;
extern long Z502_REG6;
extern long Z502_REG7;
extern long Z502_REG8;
extern long Z502_REG9;
extern INT16 Z502_MODE;

/*      Prototypes for internally called routines.                  */

void test1x(void);
void test2hx(void);
void ErrorExpected(INT32, char[]);
void SuccessExpected(INT32, char[]);
void get_skewed_random_number(long *, long);

/**************************************************************************

 Test0

 Exercises GET_TIME_OF_DAY and TERMINATE_PROCESS

 Z502_REG1              Time returned from call
 Z502_REG9              Error returned

 **************************************************************************/

void test0(void)
{
    printf("This is Release %s:  Test 0\n", CURRENT_REL);
    GET_TIME_OF_DAY(&Z502_REG1);

    printf("Time of day is %ld\n", Z502_REG1);
    TERMINATE_PROCESS(-1, &Z502_REG9);

    // We should never get to this line since the TERMINATE_PROCESS call
    // should cause the program to end.
    printf("ERROR: Test should be terminated but isn't.\n");
} // End of test0

/**************************************************************************
 Test1a

 Exercises GET_TIME_OF_DAY and SLEEP and TERMINATE_PROCESS
 What should happen here is that the difference between the time1 and time2
 will be GREATER than SleepTime.  This is because a timer interrupt takes
 AT LEAST the time specified.

 Z502_REG9              Error returned

 **************************************************************************/

void test1a(void)
{
    static long SleepTime = 100;
    static INT32 time1, time2;

    printf("This is Release %s:  Test 1a\n", CURRENT_REL);
    GET_TIME_OF_DAY(&time1);

    SLEEP(SleepTime);

    GET_TIME_OF_DAY(&time2);

    printf("Sleep Time = %ld, elapsed time= %d\n", SleepTime, time2 - time1);
    TERMINATE_PROCESS(-1, &Z502_REG9);

    printf("ERROR: Test should be terminated but isn't.\n");

} /* End of test1a    */

/**************************************************************************
 Test1b

 Exercises the CREATE_PROCESS and GET_PROCESS_ID  commands.

 This test tries lots of different inputs for create_process.
 In particular, there are tests for each of the following:

 1. use of illegal priorities
 2. use of a process name of an already existing process.
 3. creation of a LARGE number of processes, showing that
 there is a limit somewhere ( you run out of some
 resource ) in which case you take appropriate action.

 Test the following items for get_process_id:

 1. Various legal process id inputs.
 2. An illegal/non-existant name.

 Z502_REG1, _2          Used as return of process id's.
 Z502_REG3              Cntr of # of processes created.
 Z502_REG9              Used as return of error code.
 **************************************************************************/

#define         ILLEGAL_PRIORITY                -3
#define         LEGAL_PRIORITY                  10

void test1b(void)
{
    static char process_name[16];

    // Try to create a process with an illegal priority.
    printf("This is Release %s:  Test 1b\n", CURRENT_REL);
    CREATE_PROCESS("test1b_a", test1x, ILLEGAL_PRIORITY, &Z502_REG1,
                   &Z502_REG9);
    ErrorExpected(Z502_REG9, "CREATE_PROCESS");

    // Create two processes with same name - 1st succeeds, 2nd fails
    // Then terminate the process that has been created
    CREATE_PROCESS("two_the_same", test1x, LEGAL_PRIORITY, &Z502_REG2,
                   &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");
    CREATE_PROCESS("two_the_same", test1x, LEGAL_PRIORITY, &Z502_REG1,
                   &Z502_REG9);
    ErrorExpected(Z502_REG9, "CREATE_PROCESS");
    TERMINATE_PROCESS(Z502_REG2, &Z502_REG9);
    SuccessExpected(Z502_REG9, "TERMINATE_PROCESS");

    // Loop until an error is found on the create_process.
    // Since the call itself is legal, we must get an error
    // because we exceed some limit.
    Z502_REG9 = ERR_SUCCESS;
    while (Z502_REG9 == ERR_SUCCESS)
    {
        Z502_REG3++; /* Generate next unique program name*/
        sprintf(process_name, "Test1b_%ld", Z502_REG3);
        printf("Creating process \"%s\"\n", process_name);
        CREATE_PROCESS(process_name, test1x, LEGAL_PRIORITY, &Z502_REG1,
                       &Z502_REG9);
    }

    //  When we get here, we've created all the processes we can.
    //  So the OS should have given us an error
    ErrorExpected(Z502_REG9, "CREATE_PROCESS");
    printf("%ld processes were created in all.\n", Z502_REG3);

    //      Now test the call GET_PROCESS_ID for ourselves
    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9); // Legal
    SuccessExpected(Z502_REG9, "GET_PROCESS_ID");
    printf("The PID of this process is %ld\n", Z502_REG2);

    // Try GET_PROCESS_ID on another existing process
    strcpy(process_name, "Test1b_1");
    GET_PROCESS_ID(process_name, &Z502_REG1, &Z502_REG9); /* Legal */
    SuccessExpected(Z502_REG9, "GET_PROCESS_ID");
    printf("The PID of target process is %ld\n", Z502_REG1);

    // Try GET_PROCESS_ID on a non-existing process
    GET_PROCESS_ID("bogus_name", &Z502_REG1, &Z502_REG9); // Illegal
    ErrorExpected(Z502_REG9, "GET_PROCESS_ID");

    GET_TIME_OF_DAY(&Z502_REG4);
    printf("Test1b, PID %ld, Ends at Time %ld\n", Z502_REG2, Z502_REG4);
    TERMINATE_PROCESS(-2, &Z502_REG9)

} // End of test1b

/**************************************************************************
 Test1c

 Tests multiple copies of test1x running simultaneously.
 Test1c runs these with the same priority in order to show FCFS scheduling
 behavior; Test1d uses different priorities in order to show priority
 scheduling.

 WARNING:  This test assumes tests 1a - 1b run successfully

 Z502_REG1, 2, 3, 4, 5  Used as return of process id's.
 Z502_REG6              Return of PID on GET_PROCESS_ID
 Z502_REG9              Used as return of error code.

 **************************************************************************/
#define         PRIORITY1C              10

void test1c(void)
{
    static long sleep_time = 1000;

    printf("This is Release %s:  Test 1c\n", CURRENT_REL);
    CREATE_PROCESS("test1c_a", test1x, PRIORITY1C, &Z502_REG1, &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");

    CREATE_PROCESS("test1c_b", test1x, PRIORITY1C, &Z502_REG2, &Z502_REG9);

    CREATE_PROCESS("test1c_c", test1x, PRIORITY1C, &Z502_REG3, &Z502_REG9);

    CREATE_PROCESS("test1c_d", test1x, PRIORITY1C, &Z502_REG4, &Z502_REG9);

    CREATE_PROCESS("test1c_e", test1x, PRIORITY1C, &Z502_REG5, &Z502_REG9);

    // Now we sleep, see if one of the five processes has terminated, and
    // continue the cycle until one of them is gone.  This allows the test1x
    // processes to exhibit scheduling.
    // We know that the process terminated when we do a GET_PROCESS_ID and
    // receive an error on the system call.

    Z502_REG9 = ERR_SUCCESS;
    while (Z502_REG9 == ERR_SUCCESS)
    {
        SLEEP(sleep_time);
        GET_PROCESS_ID("test1c_e", &Z502_REG6, &Z502_REG9);
    }

    TERMINATE_PROCESS(-2, &Z502_REG9); /* Terminate all */

} // End test1c

/**************************************************************************
 Test 1d

 Tests multiple copies of test1x running simultaneously.
 Test1c runs these with the same priority in order to show
 FCFS scheduling behavior; Test1d uses different priorities
 in order to show priority scheduling.

 WARNING:  This test assumes tests 1a - 1b run successfully

 Z502_REG1, 2, 3, 4, 5  Used as return of process id's.
 Z502_REG6              Return of PID on GET_PROCESS_ID
 Z502_REG9              Used as return of error code.

 **************************************************************************/

#define         PRIORITY1               10
#define         PRIORITY2               11
#define         PRIORITY3               11
#define         PRIORITY4               90
#define         PRIORITY5               40

void test1d(void)
{
    static long sleep_time = 1000;

    printf("This is Release %s:  Test 1d\n", CURRENT_REL);
    CREATE_PROCESS("test1d_1", test1x, PRIORITY1, &Z502_REG1, &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");

    CREATE_PROCESS("test1d_2", test1x, PRIORITY2, &Z502_REG2, &Z502_REG9);

    CREATE_PROCESS("test1d_3", test1x, PRIORITY3, &Z502_REG3, &Z502_REG9);

    CREATE_PROCESS("test1d_4", test1x, PRIORITY4, &Z502_REG4, &Z502_REG9);

    CREATE_PROCESS("test1d_5", test1x, PRIORITY5, &Z502_REG5, &Z502_REG9);

    // Now we sleep, see if one of the five processes has terminated, and
    // continue the cycle until one of them is gone.  This allows the test1x
    // processes to exhibit scheduling.
    // We know that the process terminated when we do a GET_PROCESS_ID and
    // receive an error on the system call.

    Z502_REG9 = ERR_SUCCESS;
    while (Z502_REG9 == ERR_SUCCESS)
    {
        SLEEP(sleep_time);
        GET_PROCESS_ID("test1d_4", &Z502_REG6, &Z502_REG9);
    }

    TERMINATE_PROCESS(-2, &Z502_REG9);

} // End test1d

/**************************************************************************
 Test 1e

 Exercises the SUSPEND_PROCESS and RESUME_PROCESS commands

 This test should try lots of different inputs for suspend and resume.
 In particular, there should be tests for each of the following:

 1. use of illegal process id.
 2. what happens when you suspend yourself - is it legal?  The answer
 to this depends on the OS architecture and is up to the developer.
 3. suspending an already suspended process.
 4. resuming a process that's not suspended.

 there are probably lots of other conditions possible.

 Z502_REG1              Target process ID
 Z502_REG2              OUR process ID
 Z502_REG9              Error returned

 **************************************************************************/
#define         LEGAL_PRIORITY_1E               10

void test1e(void)
{

    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9);
    printf("Release %s:Test 1e: Pid %ld\n", CURRENT_REL, Z502_REG2);

    // Make a legal target process
    CREATE_PROCESS("test1e_a", test1x, LEGAL_PRIORITY_1E, &Z502_REG1,
                   &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");

    // Try to Suspend an Illegal PID
    SUSPEND_PROCESS((INT32) 9999, &Z502_REG9);
    ErrorExpected(Z502_REG9, "SUSPEND_PROCESS");

    // Try to Resume an Illegal PID
    RESUME_PROCESS((INT32) 9999, &Z502_REG9);
    ErrorExpected(Z502_REG9, "RESUME_PROCESS");

    // Suspend alegal PID
    SUSPEND_PROCESS(Z502_REG1, &Z502_REG9);
    SuccessExpected(Z502_REG9, "SUSPEND_PROCESS");

    // Suspend already suspended PID
    SUSPEND_PROCESS(Z502_REG1, &Z502_REG9);
    ErrorExpected(Z502_REG9, "SUSPEND_PROCESS");

    // Do a legal resume of the process we have suspended
    RESUME_PROCESS(Z502_REG1, &Z502_REG9);
    SuccessExpected(Z502_REG9, "RESUME_PROCESS");

    // Resume an already resumed process
    RESUME_PROCESS(Z502_REG1, &Z502_REG9);
    ErrorExpected(Z502_REG9, "RESUME_PROCESS");

    // Try to resume ourselves
    RESUME_PROCESS(Z502_REG2, &Z502_REG9);
    ErrorExpected(Z502_REG9, "RESUME_PROCESS");

    // It may or may not be legal to suspend ourselves;
    // architectural decision.   It can be a useful technique
    // as a way to pass off control to another process.
    SUSPEND_PROCESS(-1, &Z502_REG9);

    /* If we returned "SUCCESS" here, then there is an inconsistency;
     * success implies that the process was suspended.  But if we
     * get here, then we obviously weren't suspended.  Therefore
     * this must be an error.                                    */
    ErrorExpected(Z502_REG9, "SUSPEND_PROCESS");

    GET_TIME_OF_DAY(&Z502_REG4);
    printf("Test1e, PID %ld, Ends at Time %ld\n", Z502_REG2, Z502_REG4);

    TERMINATE_PROCESS(-2, &Z502_REG9);
} // End of test1e

/**************************************************************************
 Test1f

 Successfully suspend and resume processes. This assumes that Test1e
 runs successfully.

 In particular, show what happens to scheduling when processes
 are temporarily suspended.

 This test works by starting up a number of processes at different
 priorities.  Then some of them are suspended.  Then some are resumed.

 Z502_REG1              Loop counter
 Z502_REG2              OUR process ID
 Z502_REG3,4,5,6,7      Target process ID
 Z502_REG9              Error returned

 **************************************************************************/
#define         PRIORITY_1F1             5
#define         PRIORITY_1F2            10
#define         PRIORITY_1F3            15
#define         PRIORITY_1F4            20
#define         PRIORITY_1F5            25

void test1f(void)
{

    static long sleep_time = 300;
    int iterations;

    // Get OUR PID
    Z502_REG1 = 0; // Initialize
    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9);

    // Make legal targets
    printf("Release %s:Test 1f: Pid %ld\n", CURRENT_REL, Z502_REG2);
    CREATE_PROCESS("test1f_a", test1x, PRIORITY_1F1, &Z502_REG3, &Z502_REG9);

    CREATE_PROCESS("test1f_b", test1x, PRIORITY_1F2, &Z502_REG4, &Z502_REG9);

    CREATE_PROCESS("test1f_c", test1x, PRIORITY_1F3, &Z502_REG5, &Z502_REG9);

    CREATE_PROCESS("test1f_d", test1x, PRIORITY_1F4, &Z502_REG6, &Z502_REG9);

    CREATE_PROCESS("test1f_e", test1x, PRIORITY_1F5, &Z502_REG7, &Z502_REG9);

    // Let the 5 processes go for a while
    SLEEP(sleep_time);

    // Do a set of suspends/resumes four times
    for (iterations = 0; iterations < 4; iterations++)
    {
        // Suspend 3 of the pids and see what happens - we should see
        // scheduling behavior where the processes are yanked out of the
        // ready and the waiting states, and placed into the suspended state.

        SUSPEND_PROCESS(Z502_REG3, &Z502_REG9);
        SUSPEND_PROCESS(Z502_REG5, &Z502_REG9);
        SUSPEND_PROCESS(Z502_REG7, &Z502_REG9);

        // Sleep so we can watch the scheduling action
        SLEEP(sleep_time);

        RESUME_PROCESS(Z502_REG3, &Z502_REG9);
        RESUME_PROCESS(Z502_REG5, &Z502_REG9);
        RESUME_PROCESS(Z502_REG7, &Z502_REG9);
    }

    //   Wait for children to finish, then quit
    SLEEP((INT32) 10000);
    TERMINATE_PROCESS(-2, &Z502_REG9);

} // End of test1f

/**************************************************************************
 Test1g

 Generate lots of errors for CHANGE_PRIORITY

 Try lots of different inputs: In particular, some of the possible
 inputs include:

 1. use of illegal priorities
 2. use of an illegal process id.


 Z502_REG1              Target process ID
 Z502_REG2              OUR process ID
 Z502_REG9              Error returned

 **************************************************************************/
#define         LEGAL_PRIORITY_1G               10
#define         ILLEGAL_PRIORITY_1G            999

void test1g(void)
{

    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9);
    printf("Release %s:Test 1g: Pid %ld\n", CURRENT_REL, Z502_REG2);

    // Make a legal target
    CREATE_PROCESS("test1g_a", test1x, LEGAL_PRIORITY_1G, &Z502_REG1,
                   &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");

    // Target Illegal PID
    CHANGE_PRIORITY((INT32) 9999, LEGAL_PRIORITY_1G, &Z502_REG9);
    ErrorExpected(Z502_REG9, "CHANGE_PRIORITY");

    // Use illegal priority
    CHANGE_PRIORITY(Z502_REG1, ILLEGAL_PRIORITY_1G, &Z502_REG9);
    ErrorExpected(Z502_REG9, "CHANGE_PRIORITY");

    // Use legal priority on legal process
    CHANGE_PRIORITY(Z502_REG1, LEGAL_PRIORITY_1G, &Z502_REG9);
    SuccessExpected(Z502_REG9, "CHANGE_PRIORITY");
    // Terminate all existing processes
    TERMINATE_PROCESS(-2, &Z502_REG9);

} // End of test1g

/**************************************************************************

 Test1h  Successfully change the priority of a process

 There are TWO ways wee can see that the priorities have changed:
 1. When you change the priority, it should be possible to see
 the scheduling behaviour of the system change; processes
 that used to be scheduled first are no longer first.  This will be
 visible in the ready Q of shown by the scheduler printer.
 2. The processes with more favorable priority should schedule first so
 they should finish first.


 Z502_REG2              OUR process ID
 Z502_REG3 - 5          Target process IDs
 Z502_REG9              Error returned

 **************************************************************************/

#define         MOST_FAVORABLE_PRIORITY         1
#define         FAVORABLE_PRIORITY             10
#define         NORMAL_PRIORITY                20
#define         LEAST_FAVORABLE_PRIORITY       30

void test1h(void)
{
    long ourself;

    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9);

    // Make our priority high
    printf("Release %s:Test 1h: Pid %ld\n", CURRENT_REL, Z502_REG2);
    ourself = -1;
    CHANGE_PRIORITY(ourself, MOST_FAVORABLE_PRIORITY, &Z502_REG9);

    // Make legal targets
    CREATE_PROCESS("test1h_a", test1x, NORMAL_PRIORITY, &Z502_REG3,
                   &Z502_REG9);
    CREATE_PROCESS("test1h_b", test1x, NORMAL_PRIORITY, &Z502_REG4,
                   &Z502_REG9);
    CREATE_PROCESS("test1h_c", test1x, NORMAL_PRIORITY, &Z502_REG5,
                   &Z502_REG9);

    //      Sleep awhile to watch the scheduling
    SLEEP(200);

    //  Now change the priority - it should be possible to see
    //  that the priorities have been changed for processes that
    //  are ready and for processes that are sleeping.

    CHANGE_PRIORITY(Z502_REG3, FAVORABLE_PRIORITY, &Z502_REG9);

    CHANGE_PRIORITY(Z502_REG5, LEAST_FAVORABLE_PRIORITY, &Z502_REG9);

    //      Sleep awhile to watch the scheduling
    SLEEP(200);

    //  Now change the priority - it should be possible to see
    //  that the priorities have been changed for processes that
    //  are ready and for processes that are sleeping.

    CHANGE_PRIORITY(Z502_REG3, LEAST_FAVORABLE_PRIORITY, &Z502_REG9);

    CHANGE_PRIORITY(Z502_REG4, FAVORABLE_PRIORITY, &Z502_REG9);

    //     Sleep awhile to watch the scheduling
    SLEEP(600);

    // Terminate everyone
    TERMINATE_PROCESS(-2, &Z502_REG9);

} // End of test1h  

#define         NUMBER_OF_TEST1X_ITERATIONS     10

void test1x(void)
{
    long RandomSleep = 17;
    int Iterations;

    GET_PROCESS_ID("", &Z502_REG2, &Z502_REG9);
    printf("Release %s:Test 1x: Pid %ld\n", CURRENT_REL, Z502_REG2);

    for (Iterations = 0; Iterations < NUMBER_OF_TEST1X_ITERATIONS;
            Iterations++)
    {
        GET_TIME_OF_DAY(&Z502_REG3);
        RandomSleep = (RandomSleep * Z502_REG3) % 143;
        SLEEP(RandomSleep);
        GET_TIME_OF_DAY(&Z502_REG4);
        printf("Test1X: Pid = %d, Sleep Time = %ld, Latency Time = %d\n",
               (int) Z502_REG2, RandomSleep, (int) (Z502_REG4 - Z502_REG3));
    }
    printf("Test1x, PID %ld, Ends at Time %ld\n", Z502_REG2, Z502_REG4);

    TERMINATE_PROCESS(-1, &Z502_REG9);
    printf("ERROR: Test1x should be terminated but isn't.\n");

} /* End of test1x    */

/**************************************************************************
 ErrorExpected    and    SuccessExpected

 These routines simply handle the display of success/error data.

 **************************************************************************/

void ErrorExpected(INT32 ErrorCode, char sys_call[])
{
    if (ErrorCode == ERR_SUCCESS)
    {
        printf("An Error SHOULD have occurred.\n");
        printf("????: Error( %d ) occurred in case %d (%s)\n", ErrorCode,
               Z502_PROGRAM_COUNTER - 2, sys_call);
    }
    else
        printf("Program correctly returned an error: %d\n", ErrorCode);

} // End of ErrorExpected

void SuccessExpected(INT32 ErrorCode, char sys_call[])
{
    if (ErrorCode != ERR_SUCCESS)
    {
        printf("An Error should NOT have occurred.\n");
        printf("????: Error( %d ) occurred in case %d (%s)\n", ErrorCode,
               Z502_PROGRAM_COUNTER - 2, sys_call);
    }
    else
        printf("Program correctly returned success.\n");

} // End of SuccessExpected

/**************************************************************************
 Test2a exercises a simple memory write and read

 Use:  Z502_REG1                data_written
 Z502_REG2                data_read
 Z502_REG3                address
 Z502_REG4                process_id
 Z502_REG9                error

 In global.h, there's a variable  DO_MEMORY_DEBUG.   Switching it to
 TRUE will allow you to see what the memory system thinks is happening.
 WARNING - it's verbose -- and I don't want to see such output - it's
 strictly for your debugging pleasure.
 **************************************************************************/
void test2a(void)
{

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG9);

    printf("Release %s:Test 2a: Pid %ld\n", CURRENT_REL, Z502_REG4);
    Z502_REG3 = 412;
    Z502_REG1 = Z502_REG3 + Z502_REG4;
    MEM_WRITE(Z502_REG3, &Z502_REG1);

    MEM_READ(Z502_REG3, &Z502_REG2);

    printf("PID= %ld  address= %ld   written= %ld   read= %ld\n", Z502_REG4,
           Z502_REG3, Z502_REG1, Z502_REG2);
    if (Z502_REG2 != Z502_REG1)
        printf("AN ERROR HAS OCCURRED.\n");
    TERMINATE_PROCESS(-1, &Z502_REG9);

} // End of test2a   

/**************************************************************************
 Test2b

 Exercises simple memory writes and reads.  Watch out, the addresses 
 used are diabolical and are designed to show unusual features of your 
 memory management system.

 Use:  
 Z502_REG1                data_written
 Z502_REG2                data_read
 Z502_REG3                address
 Z502_REG4                process_id
 Z502_REG5                test_data_index
 Z502_REG9                error

 The following registers are used for sanity checks - after each
 read/write pair, we will read back the first set of data to make
 sure it's still there.

 Z502_REG6                First data written
 Z502_REG7                First data read
 Z502_REG8                First address

 **************************************************************************/

#define         TEST_DATA_SIZE          (INT16)7

void test2b(void)
{
    static INT32 test_data[TEST_DATA_SIZE ] = {0, 4, PGSIZE - 2, PGSIZE, 3
        * PGSIZE - 2, (VIRTUAL_MEM_PGS - 1) * PGSIZE, VIRTUAL_MEM_PGS
        * PGSIZE - 2};

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG9);
    printf("\n\nRelease %s:Test 2b: Pid %ld\n", CURRENT_REL, Z502_REG4);

    Z502_REG8 = 5 * PGSIZE;
    Z502_REG6 = Z502_REG8 + Z502_REG4 + 7;
    MEM_WRITE(Z502_REG8, &Z502_REG6);

    // Loop through all the memory addresses defined
    while (TRUE)
    {
        Z502_REG3 = test_data[(INT16) Z502_REG5];
        Z502_REG1 = Z502_REG3 + Z502_REG4 + 27;
        MEM_WRITE(Z502_REG3, &Z502_REG1);

        MEM_READ(Z502_REG3, &Z502_REG2);

        printf("PID= %ld  address= %ld  written= %ld   read= %ld\n", Z502_REG4,
               Z502_REG3, Z502_REG1, Z502_REG2);
        if (Z502_REG2 != Z502_REG1)
            printf("AN ERROR HAS OCCURRED.\n");

        //      Go back and check earlier write
        MEM_READ(Z502_REG8, &Z502_REG7);

        printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
               Z502_REG4, Z502_REG8, Z502_REG6, Z502_REG7);
        if (Z502_REG6 != Z502_REG7)
            printf("AN ERROR HAS OCCURRED.\n");
        Z502_REG5++;
    }
} // End of test2b    

/**************************************************************************

 Test2c causes usage of disks.  The test is designed to give
 you a chance to develop a mechanism for handling disk requests.

 Z502_REG3  - address where data was written/read.
 Z502_REG4  - process id of this process.
 Z502_REG6  - number of iterations/loops through the code.
 Z502_REG7  - which page will the write/read be on. start at 0
 Z502_REG9  - returned error code.

 You will need a way to get the data read back from the disk into the
 buffer defined by the user process.  This can most easily be done after
 the process is rescheduled and about to return to user code.
 **************************************************************************/

#define         DISPLAY_GRANULARITY2c           10
#define         TEST2C_LOOPS                    50

typedef union
{
    char char_data[PGSIZE ];
    UINT32 int_data[PGSIZE / sizeof (int)];
} DISK_DATA;

void test2c(void)
{
    DISK_DATA *data_written;
    DISK_DATA *data_read;
    long disk_id;
    INT32 sanity = 1234;
    long sector;
    int Iterations;

    data_written = (DISK_DATA *) calloc(1, sizeof (DISK_DATA));
    data_read = (DISK_DATA *) calloc(1, sizeof (DISK_DATA));
    if (data_read == 0)
        printf("Something screwed up allocating space in test2c\n");

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG9);

    sector = Z502_REG4;
    printf("\n\nRelease %s:Test 2c: Pid %ld\n", CURRENT_REL, Z502_REG4);

    for (Iterations = 0; Iterations < TEST2C_LOOPS; Iterations++)
    {
        // Pick some location on the disk to write to
        disk_id = (Z502_REG4 / 2) % MAX_NUMBER_OF_DISKS + 1;
        sector = (sector * 177) % NUM_LOGICAL_SECTORS;
        data_written->int_data[0] = disk_id;
        data_written->int_data[1] = sanity;
        data_written->int_data[2] = sector;
        data_written->int_data[3] = (int) Z502_REG4;
        DISK_WRITE(disk_id, sector, (char*) (data_written->char_data));

        // Now read back the same data.  Note that we assume the
        // disk_id and sector have not been modified by the previous
        // call.
        DISK_READ(disk_id, sector, (char*) (data_read->char_data));

        if ((data_read->int_data[0] != data_written->int_data[0])
                || (data_read->int_data[1] != data_written->int_data[1])
                || (data_read->int_data[2] != data_written->int_data[2])
                || (data_read->int_data[3] != data_written->int_data[3]))
        {
            printf("AN ERROR HAS OCCURRED.\n");
        }
        else if (Z502_REG6 % DISPLAY_GRANULARITY2c == 0)
        {
            printf("SUCCESS READING  PID= %ld  disk_id =%ld, sector = %ld\n",
                   Z502_REG4, disk_id, sector);
        }
    } // End of for loop

    // Now read back the data we've written and paged

    printf("Reading back data: test 2c, PID %ld.\n", Z502_REG4);
    sector = Z502_REG4;

    for (Iterations = 0; Iterations < TEST2C_LOOPS; Iterations++)
    {
        disk_id = (Z502_REG4 / 2) % MAX_NUMBER_OF_DISKS + 1;
        sector = (sector * 177) % NUM_LOGICAL_SECTORS;
        data_written->int_data[0] = disk_id;
        data_written->int_data[1] = sanity;
        data_written->int_data[2] = sector;
        data_written->int_data[3] = Z502_REG4;

        DISK_READ(disk_id, sector, (char*) (data_read->char_data));

        if ((data_read->int_data[0] != data_written->int_data[0])
                || (data_read->int_data[1] != data_written->int_data[1])
                || (data_read->int_data[2] != data_written->int_data[2])
                || (data_read->int_data[3] != data_written->int_data[3]))
        {
            printf("AN ERROR HAS OCCURRED.\n");
        }
        else if (Z502_REG6 % DISPLAY_GRANULARITY2c == 0)
        {
            printf("SUCCESS READING  PID= %ld  disk_id =%ld, sector = %ld\n",
                   Z502_REG4, disk_id, sector);
        }

    } // End of for loop

    GET_TIME_OF_DAY(&Z502_REG8);
    printf("Test2c, PID %ld, Ends at Time %ld\n", Z502_REG4, Z502_REG8);
    TERMINATE_PROCESS(-1, &Z502_REG9);

} // End of test2c    

/**************************************************************************

 Test2d runs several disk programs at a time.  The purpose here
 is to watch the scheduling that goes on for these
 various disk processes.  The behavior that should be seen
 is that the processes alternately run and do disk
 activity - there should always be someone running unless
 ALL processes happen to be waiting on the disk at some
 point.
 This program will terminate when all the test2c routines
 have finished.

 Z502_REG4  - process id of this process.
 Z502_REG5  - returned error code.
 Z502_REG6  - pid of target process.
 Z502_REG8  - returned error code from the GET_PROCESS_ID call.

 **************************************************************************/
#define           MOST_FAVORABLE_PRIORITY                       1

void test2d(void)
{
    static INT32 trash;

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG5);
    printf("\n\nRelease %s:Test 2d: Pid %ld\n", CURRENT_REL, Z502_REG4);
    CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &Z502_REG9);

    CREATE_PROCESS("first", test2c, 5, &trash, &Z502_REG5);
    CREATE_PROCESS("second", test2c, 5, &trash, &Z502_REG5);
    CREATE_PROCESS("third", test2c, 7, &trash, &Z502_REG5);
    CREATE_PROCESS("fourth", test2c, 7, &trash, &Z502_REG5);
    CREATE_PROCESS("fifth", test2c, 7, &trash, &Z502_REG5);

    SLEEP(50000);

    TERMINATE_PROCESS(-2, &Z502_REG5);

} // End of test2d 

/**************************************************************************

 Test2e causes extensive page replacement.  It simply advances through 
 virtual memory.  It will eventually end because using an illegal virtual 
 address will cause this process to be terminated by the operating system.

 Z502_REG1  - data that was written.
 Z502_REG2  - data that was read from memory.
 Z502_REG3  - address where data was written/read.
 Z502_REG4  - process id of this process.
 Z502_REG6  - number of iterations/loops through the code.
 Z502_REG9  - returned error code.

 **************************************************************************/

#define         STEP_SIZE               VIRTUAL_MEM_PGS/(2 * PHYS_MEM_PGS )
#define         DISPLAY_GRANULARITY2e     16 * STEP_SIZE

void test2e(void)
{
    int Iterations;

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG9);
    printf("\n\nRelease %s:Test 2e: Pid %ld\n", CURRENT_REL, Z502_REG4);

    for (Iterations = 0; Iterations < VIRTUAL_MEM_PGS; Iterations +=
            STEP_SIZE)
    {
        Z502_REG3 = PGSIZE * Iterations; // Generate address
        Z502_REG1 = Z502_REG3 + Z502_REG4; // Generate data 
        MEM_WRITE(Z502_REG3, &Z502_REG1); // Write the data

        MEM_READ(Z502_REG3, &Z502_REG2); // Read back data

        if (Iterations % DISPLAY_GRANULARITY2e == 0)
            printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
                   Z502_REG4, Z502_REG3, Z502_REG1, Z502_REG2);
        if (Z502_REG2 != Z502_REG1) // Written = read?
            printf("AN ERROR HAS OCCURRED.\n");

        // It makes life more fun!! to write the data again
        MEM_WRITE(Z502_REG3, &Z502_REG1); // Write the data

    } // End of for loop

    // Now read back the data we've written and paged
    printf("Reading back data: test 2e, PID %ld.\n", Z502_REG4);
    for (Iterations = 0; Iterations < VIRTUAL_MEM_PGS; Iterations +=
            STEP_SIZE)
    {

        Z502_REG3 = PGSIZE * Iterations; // Generate address
        Z502_REG1 = Z502_REG3 + Z502_REG4; // Data expected
        MEM_READ(Z502_REG3, &Z502_REG2); // Read back data

        if (Iterations % DISPLAY_GRANULARITY2e == 0)
            printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
                   Z502_REG4, Z502_REG3, Z502_REG1, Z502_REG2);
        if (Z502_REG2 != Z502_REG1) // Written = read?
            printf("AN ERROR HAS OCCURRED.\n");

    } // End of for loop
    TERMINATE_PROCESS(-2, &Z502_REG5); // Added 12/1/2013
} // End of test2e    

/**************************************************************************

 Test2f causes extensive page replacement, but reuses pages.
 This program will terminate, but it might take a while.

 Z502_REG1  - data that was written.
 Z502_REG2  - data that was read from memory.
 Z502_REG3  - address where data was written/read.
 Z502_REG4  - process id of this process.
 Z502_REG5  - holds the pointer to the record of page touches
 Z502_REG9  - returned error code.

 **************************************************************************/

#define                 NUMBER_OF_ITERATIONS            3
#define                 LOOP_COUNT                    400
#define                 DISPLAY_GRANULARITY2          100
#define                 LOGICAL_PAGES_TO_TOUCH       2 * PHYS_MEM_PGS

typedef struct
{
    INT16 page_touched[LOOP_COUNT]; // Bugfix Rel 4.03  12/1/2013
} MEMORY_TOUCHED_RECORD;

void test2f(void)
{
    MEMORY_TOUCHED_RECORD *mtr;
    short Iterations, Index, Loops;

    mtr = (MEMORY_TOUCHED_RECORD *) calloc(1, sizeof (MEMORY_TOUCHED_RECORD));

    GET_PROCESS_ID("", &Z502_REG4, &Z502_REG9);
    printf("\n\nRelease %s:Test 2f: Pid %ld\n", CURRENT_REL, Z502_REG4);

    for (Iterations = 0; Iterations < NUMBER_OF_ITERATIONS; Iterations++)
    {
        for (Index = 0; Index < LOOP_COUNT; Index++) // Bugfix Rel 4.03  12/1/2013
            mtr->page_touched[Index] = 0;
        for (Loops = 0; Loops < LOOP_COUNT; Loops++)
        {
            // Get a random page number
            get_skewed_random_number(&Z502_REG7, LOGICAL_PAGES_TO_TOUCH);
            Z502_REG3 = PGSIZE * Z502_REG7; // Convert page to addr.
            Z502_REG1 = Z502_REG3 + Z502_REG4; // Generate data for page
            MEM_WRITE(Z502_REG3, &Z502_REG1);
            // Write it again, just as a test
            MEM_WRITE(Z502_REG3, &Z502_REG1);

            // Read it back and make sure it's the same
            MEM_READ(Z502_REG3, &Z502_REG2);
            if (Loops % DISPLAY_GRANULARITY2 == 0)
                printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
                       Z502_REG4, Z502_REG3, Z502_REG1, Z502_REG2);
            if (Z502_REG2 != Z502_REG1)
                printf("AN ERROR HAS OCCURRED: READ NOT EQUAL WRITE.\n");

            // Record in our data-base that we've accessed this page
            mtr->page_touched[(short) Loops] = Z502_REG7;

        } // End of for Loops

        for (Loops = 0; Loops < LOOP_COUNT; Loops++)
        {

            // We can only read back from pages we've previously
            // written to, so find out which pages those are.
            Z502_REG6 = mtr->page_touched[(short) Loops];
            Z502_REG3 = PGSIZE * Z502_REG6; // Convert page to addr.
            Z502_REG1 = Z502_REG3 + Z502_REG4; // Expected read
            MEM_READ(Z502_REG3, &Z502_REG2);

            if (Loops % DISPLAY_GRANULARITY2 == 0)
                printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
                       Z502_REG4, Z502_REG3, Z502_REG1, Z502_REG2);
            if (Z502_REG2 != Z502_REG1)
                printf("ERROR HAS OCCURRED: READ NOT SAME AS WRITE.\n");
        } // End of for Loops

        // We've completed reading back everything
        printf("TEST 2f, PID %ld, HAS COMPLETED %d ITERATIONS\n", Z502_REG4,
               Iterations);
    } // End of for Iterations

    TERMINATE_PROCESS(-1, &Z502_REG9);

} // End of test2f

/**************************************************************************
 Test2g

 Tests multiple copies of test2f running simultaneously.
 Test2f runs these with the same priority in order to show
 equal preference for each child process.  This means all the
 child processes will be stealing memory from each other.

 WARNING:  This test assumes tests 2e - 2f run successfully

 Z502_REG1, 2, 3, 4, 5  Used as return of process id's.
 Z502_REG6              Return of PID on GET_PROCESS_ID
 Z502_REG9              Used as return of error code.

 **************************************************************************/

#define         PRIORITY2G              10

void test2g(void)
{
    static long sleep_time = 1000;

    printf("This is Release %s:  Test 2g\n", CURRENT_REL);
    CREATE_PROCESS("test2g_a", test2f, PRIORITY2G, &Z502_REG1, &Z502_REG9);
    CREATE_PROCESS("test2g_b", test2f, PRIORITY2G, &Z502_REG2, &Z502_REG9);
    CREATE_PROCESS("test2g_c", test2f, PRIORITY2G, &Z502_REG3, &Z502_REG9);
    CREATE_PROCESS("test2g_d", test2f, PRIORITY2G, &Z502_REG4, &Z502_REG9);
    CREATE_PROCESS("test2g_e", test2f, PRIORITY2G, &Z502_REG5, &Z502_REG9);
    SuccessExpected(Z502_REG9, "CREATE_PROCESS");

    // In these next three cases, we will loop until the target
    // process ( test2g_e ) has terminated.  We know it
    // terminated because for a while we get success on the call
    // GET_PROCESS_ID, and then we get failure when the process
    // no longer exists.

    Z502_REG9 = ERR_SUCCESS;
    while (Z502_REG9 == ERR_SUCCESS)
    {
        SLEEP(sleep_time);
        GET_PROCESS_ID("test2g_e", &Z502_REG6, &Z502_REG9);
    }
    TERMINATE_PROCESS(-2, &Z502_REG9); // Terminate all

} // End test2g

/**************************************************************************

 get_skewed_random_number   Is a homegrown deterministic random
 number generator.  It produces  numbers that are NOT uniform across
 the allowed range.
 This is useful in picking page locations so that pages
 get reused and makes a LRU algorithm meaningful.
 This algorithm is VERY good for developing page replacement tests.

 **************************************************************************/

#define                 SKEWING_FACTOR          0.60

void get_skewed_random_number(long *random_number, long range)
{
    double temp;
    long extended_range = (long) pow(range, (double) (1 / SKEWING_FACTOR));

    temp = (double) rand();
    if (temp < 0)
        temp = -temp;
    temp = (double) ((long) temp % extended_range);
    temp = pow(temp, (double) SKEWING_FACTOR);
    *random_number = (long) temp;
} // End get_skewed_random_number 

/*****************************************************************
 testStartCode()
 A new thread (other than the initial thread) comes here the
 first time it's scheduled.
 *****************************************************************/
void testStartCode()
{
    void (*routine)(void);
    routine = (void (*)(void)) Z502PrepareProcessForExecution();
    (*routine)();
    // If we ever get here, it's because the thread ran to the end
    // of a test program and wasn't terminated properly.
    printf("ERROR:  Simulation did not end correctly\n");
    exit(0);
}

/*****************************************************************
 main()
 This is the routine that will start running when the
 simulator is invoked.
 *****************************************************************/
int main(int argc, char *argv[])
{
    int i;
    for (i = 0; i < MAX_NUMBER_OF_USER_THREADS; i++)
    {
        Z502CreateUserThread(testStartCode);
    }

    osInit(argc, argv);
    // We should NEVER return from this routine.  The result of
    // osInit is to select a program to run, to start a process
    // to execute that program, and NEVER RETURN!
    return (-1);
} // End of main
