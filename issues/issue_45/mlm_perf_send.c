/*  =========================================================================
    mlm_perf_send.c, the poducer of messages : the sender. 

    Runs various tests on Malamute to test performance of the core engines.
    =========================================================================
*/

//  This header file gives us the Malamute APIs plus Zyre, CZMQ, and libzmq:
#include <malamute.h>

#define SCENARIOA_RATIO  2
#define SCENARIOB_RATIO  20
#define SCENARIOC_RATIO  200
#define SCENARIOD_RATIO  5


void init_msg (char *msg, int size)
{
    int char_nbr;
    for (char_nbr = 0; char_nbr < size; char_nbr++)
        msg [char_nbr] = '0' + char_nbr % 10;

    msg [size - 1] = '\0';
}

//scenario A
//Scenario A : Enqueuing $total_count messages of 1024 bytes each, then dequeuing them afterwards
void scenarioA (int total_count)
{
    char msg1024 [1024];
    init_msg (msg1024, 1024);

    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    int rc = mlm_client_connect (writer, MLM_DEFAULT_ENDPOINT, 0, "writer");
    assert (rc == 0);
    
    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_connect (reader, MLM_DEFAULT_ENDPOINT, 0, "reader");
    assert (rc == 0);
    mlm_client_set_consumer (reader, "weather", "rain.");
    
    printf ("Scenario A : Enqueuing %d messages of 1024 bytes each, then dequeuing them afterwards...\n",
            total_count);
    int count = total_count;
    int64_t start = zclock_time ();
    while (count) {
        mlm_client_sendx (writer, "weather", "rain.moscow", msg1024, NULL);
        count--;
    }
    mlm_client_sendx (writer, "weather", "rain.signal", "END", NULL);
    
    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "rain.signal"))
            break;
        zmsg_destroy (&msg);
        count++;
    }
    int64_t duration = zclock_time () - start;
    if (duration == 0)
        duration = 1;
    printf ("Scenario A : sending %d messages (1k): %d msec. %d msg/s \n",
           count, (int) (duration), (int) (1000 * total_count / duration));
    
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
}

//scenarioB
//Scenario B : Enqueuing and dequeuing simultaneously $total_count messages of 1024 bytes each
void scenarioB (int total_count)
{
    char msg1024 [1024];
    init_msg (msg1024, 1024);
    
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    int rc = mlm_client_connect (writer, MLM_DEFAULT_ENDPOINT, 0, "writer");
    assert (rc == 0);
 
    printf ("Scenario B : Enqueuing and dequeuing %d messages of 1024 bytes each...\n",total_count);
    int count = 0;
    int64_t start = zclock_time ();
    while (count != total_count) {
        char subject [20];
        sprintf (subject, "temp.%d", count);
        mlm_client_sendx (writer, "weather", subject, msg1024, NULL);
        count++;
        if ((count % 10000) == 0)
            printf (".");
    }
    if (count >= 10000)
        printf ("\n");

    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_connect (reader, MLM_DEFAULT_ENDPOINT, 0, "reader");
    assert (rc == 0);
    mlm_client_set_consumer (reader, "weather", "signal.ack");
    
    mlm_client_sendx (writer, "temp.signal", "END", NULL);
    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "signal.ack")){
            zmsg_destroy (&msg);
            break;
        }
        else
            printf ("unexpected subject %s\n", mlm_client_subject (reader));

        zmsg_destroy (&msg);
    }
    puts ("B3");
    int64_t duration = zclock_time () - start;
    if (duration == 0)
        duration = 1;
    printf ("Scenario B : sending %d messages (1k): %d msec. %d msg/s\n", total_count, (int) (duration), (int) (1000*total_count/duration));
    
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
     
}

//scenario C
//Scenario C : Enqueuing and dequeuing simultaneously $total_count messages of 32 bytes each.
void scenarioC (int total_count)
{
    char msg32 [32];
    init_msg (msg32,32);
    
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    int rc = mlm_client_connect (writer, MLM_DEFAULT_ENDPOINT, 0, "writer");
    assert (rc == 0);

    printf ("Scenario C : Enqueuing and dequeuing simultaneously %d messages of 32 bytes each...\n",total_count);
    int count = 0;
    int64_t start = zclock_time ();
    while (count!=total_count) {
        if (mlm_client_sendx (writer, "weather", "temp.moscow", msg32, NULL) !=0) {
            printf ("Error : enable to send msg (mlm_client_sendx), count=%d",count);
            break;
        };
        count++;
        if ((count % 10000) == 0)
            printf (".");
    }
    if (count >= 10000)
        printf ("\n");

    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_connect (reader, MLM_DEFAULT_ENDPOINT, 0, "reader");
    assert (rc == 0);
    mlm_client_set_consumer (reader, "weather", "signal.ack");
    mlm_client_sendx (writer, "temp.signal", "END", NULL);
    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "signal.ack")) {
            zmsg_destroy (&msg);
            break;
        }
        else
            printf ("unexpected subject %s\n", mlm_client_subject (reader));

        zmsg_destroy (&msg);
    }
    int64_t duration = zclock_time () - start;
    if (duration == 0)
        duration = 1;
    printf ("Scenario C : sending %d messages (32): %d msec. %d msg/s\n", total_count, (int) (duration), (int) (1000*total_count/duration));
    
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
    
}

//scenario D
//Scenario D : Enqueuing and dequeuing simultaneously $total_count messages of 32768  bytes each

void scenarioD (int total_count)
{
    char msg32768 [32768];
    init_msg (msg32768, 32768);
    
    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    int rc = mlm_client_connect (writer, MLM_DEFAULT_ENDPOINT, 0, "writer");
    assert (rc == 0);

    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    rc = mlm_client_connect (reader, MLM_DEFAULT_ENDPOINT, 0, "reader");
    assert (rc == 0);
    mlm_client_set_consumer (reader, "weather", "signal.ack");
        
    printf ("Scenario D : Enqueuing and dequeuing simultaneously %d messages of 32768  bytes each...\n",total_count);
    int count = 0;
    int64_t start = zclock_time ();
    while (count != total_count) {
        mlm_client_sendx (writer, "weather", "temp.moscow", msg32768, NULL);
        count++;
        if ((count % 10000) == 0)
            printf (".");
    }
    if (count >= 10000)
        printf ("\n");

    mlm_client_sendx (writer, "weather", "temp.signal", "END", NULL);
    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "signal.ack")) {
            zmsg_destroy (&msg);
            break;
        }
        else
            printf ("unexpected subject %s\n", mlm_client_subject (reader));

        zmsg_destroy (&msg);
    }
    int64_t duration = zclock_time () - start;
    if (duration == 0)
        duration = 1;
    printf ("Scenario D : sending %d messages (32k): %d msec. %d msg/s\n",
            total_count, (int) (duration), (int) (1000 * total_count / duration));
    
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
    
}

int main (int argc, char *argv [])
{
    //  We will remove all buffer limits on internal plumbing; instead,
    //  we use credit-based flow control to rate-limit traffic per client.
    //  If we allow limits, then the engines will block under stress.
    zsys_set_pipehwm (0);
    zsys_set_sndhwm (0);
    zsys_set_rcvhwm (0);

    //  Flush all test output immediately without buffering
    setbuf (stdout, NULL);

    int count = 1000;
    if (argc > 1)
        count = atoi (argv [1]);
    
//     scenarioA (SCENARIOA_RATIO * count);
    scenarioB (SCENARIOB_RATIO * count);
//     scenarioC (SCENARIOC_RATIO * count);
//     scenarioD (count / SCENARIOD_RATIO);

    return 0;
}
