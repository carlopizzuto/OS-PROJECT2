[04/10] [16:30]
I have read through the instuctions and checked out the supporting documents (sample run, c code). Overall, I am not too worried about starting the project late, I have had a lot of previous experiece with Operating Systems and C, so working with threads is not a problem. I am going to develop this in an Arch Linux env with nvim, and plan to start with simple functionalities and work my way up from there, starting with the tellers and then creating the customers.

[4/10] [21:50]
I have finished implementing the skeleton of the main program. Currently it only creates 3 teller threads and 50 customer threads, but these do not do anything except print their IDs and a couple of messages. I have been struggling a little with vim and the keybinds, which caused me to delete half of my code (twice :<), so I will start commiting more often. In this session I had some logical errors that were hard to find but easy to fix, but the overall structure of the code is good (I think). Anyways, what I wanted to implement works, but I might add a little more.

[4/11] [13:40]
I didn't modify any more code yesterday. Today, I plan to have a quick session to start implementing the code for teller / customer threads. Right now they are assigned unique IDs and only print a couple of messages, but don't use the semaphores. I plan to start by implementing the 'easiest' functionalities for the the customer threads, and then move on to the teller threads.

[4/11] [14:35]
This session I completed some of the customer code. I ran into some problems with printing the IDs of the threads inside of an 'if' statement, but I managed to find a workaround (temporary). I also created a makefile, but I would've liked to get more done. I'll try to do more today and tomorrow, but the day I will work on this the most is on Sunday (personal events this weekend).

[4/13] [14:00]
I plan on having a very long session where I'll finish everything. My plan is to finish implementing the Customer threads, and when I need to, switch to the teller threads. 
