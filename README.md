# ai

**DISCLAIMER**

  Program: AI Linux Assistant
  
  Author: Guillaume R @ GIMO
  
  Description: This program serves as a GPT-powered assistant for Linux
               systems, designed to provide intelligent and interactive 
               functionality to users. It integrates AI capabilities to 
               enhance productivity and automate tasks within a Linux 
               environment.
  
  License: This software is licensed for non-commercial use only. 
           Redistribution, modification, and use in any form for commercial 
           purposes are strictly prohibited.
  
  Disclaimer: This program is provided "as is" without any warranty of any kind,
              either express or implied, including but not limited to the 
              implied warranties of merchantability and fitness for a 
              particular purpose.
  
  Requirements: 
   - GCC Compiler
   - json-c, libcurl 
   - Linux Operating System
   - outgoing firewall access to OpenAI API servers
  
  Usage:
   1. Compile and install : make
   2. Edit /etc/ai/ai.conf and install your OpenAI key 
   3. Run the executable: ai please echo Hello world ! 
  

**AI Linux assistant for your shell needs (For sysadmins really)**

**INSTALL**

make
then edit /etc/ai/ai.conf and put in your OpenAI API key in OPENAIKEY=YOURKEYHERE

**USAGE**

It will go in interactive mode :
ai

or with command line
ai List me the 10 biggest files in /tmp
(Not that in command lines you have to avoid ' and "s and other things that would make bash error)

Works pretty well. 

**example :**

root@vultr:~/ai# ai Hi. Lets make a demo of you as an assistant that execute commands. please list the top 5 biggest files in /root
Please note that accessing the /root directory typically requires root privileges. Here's the command to list the top 5 biggest files in /root:

<CMD>sudo find /root -type f -exec du -h {} + | sort -rh | head -n 5</CMD>

I need to run this command: bash -c "sudo find /root -type f -exec du -h {} + | sort -rh | head -n 5"

Do you want to proceed? (yes/no/exit) [no]: yes

Command output:

command executed: <sudo find /root -type f -exec du -h {} + | sort -rh | head -n 5> status: <executed> output: <681M    /root/.bitcoin/chainstate/MANIFEST-000002
428M    /root/grpc/grpc/.git/objects/pack/pack-c7c16bc5828fdaf427e2d5389dabec7577e32239.pack
307M    /root/grpc/grpc/.git/modules/third_party/boringssl-with-bazel/objects/pack/pack-1b199781b5b81cabb319e735e419deebbe82c3b8.pack
237M    /root/yolo/darknet/yolov3.weights
235M    /root/.cache/puppeteer/chrome/linux-127.0.6533.72/chrome-linux64/chrome
>

The command executed successfully and listed the top 5 biggest files in the /root directory. If you need further assistance or another command, feel free to ask!

Do you want to continue the conversation? (yes/no) [no]: yes

Enter your next message: What do you think is yolov3.weight ?

The file "yolov3.weights" is likely a pre-trained model file for YOLOv3, which stands for "You Only Look Once version 3." YOLOv3 is a popular real-time object detection system that uses deep learning to identify and locate objects in images or videos. The weights file contains the parameters of the neural network that have been learned from training on a large dataset.

Do you want to continue the conversation? (yes/no) [no]:




