#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <mutex>
#include <thread>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include "SimTimer.h"
#include "PrintData.h"
#include "SimUdpSocket.h"

// unity build
#include "SimTimer.cpp"
#include "PrintData.cpp"
#include "SimUdpSocket.cpp"

const char* IP_ADDRESS       = "192.168.137.189";
const char* MY_IP_ADDRESS    = "192.168.137.190";
const char* BASE_MC_ADDRESS  = "229.7.7.0";
const int   PORT             = 4000;
const int   NUM_MC_ADDRESSES = 250;
const int   MAX_BUFFER       = 65536;

// Set the following settings to ensure traffic is recorded
// Also, make sure the PORT is allowed through the firewall
//  # increase max size of send/receive buffers to 8 MB
//  # increase max number of multicast groups to 1024
//  cp /etc/sysctl.conf /etc/sysctl.conf.orig
//  echo "net.core.rmem_max=8388608" >> /etc/sysctl.conf
//  echo "net.core.wmem_max=8388608" >> /etc/sysctl.conf
//  echo "net.core.rmem_default=8388608" >> /etc/sysctl.conf
//  echo "net.core.wmem_default=8388608" >> /etc/sysctl.conf
//  echo "net.core.optmem_max=8388608" >> /etc/sysctl.conf
//  echo "net.ipv4.igmp_max_memberships=1024" >> /etc/sysctl.conf

struct TBuffer
{
   std::string from_ip;
   std::string from_mc;
   double      time;
   uint64_t    bytes;
   char        buffer[MAX_BUFFER];
};

struct TThreadData
{
   int write_index;
   int read_index;
   int count;
   int running;
   TBuffer data[2];
   std::mutex thread_mutex;
};

struct TPlaybackFile
{
   std::string filename;
   std::string from_ip;
   std::string from_mc;
};

struct TPlaybackBuffer
{
   double   time;
   uint64_t bytes;
   char     buffer[MAX_BUFFER];
};

int         total_packets_recorded = 0;
bool        playback_running = true;
TThreadData thread_data;

void int_handler(int sig_number)
{
   if (sig_number == SIGINT)
   {
      thread_data.thread_mutex.lock();
      thread_data.running = 0;
      thread_data.thread_mutex.unlock();
      playback_running = false;
   }
}

void playback(const char* Directory)
{
   std::unordered_map<std::string, std::vector<TPlaybackFile>> files;

   printf("Playback from %s directory\n", Directory);

   // Find all the .bin files in the folder and break them up by computer
   std::filesystem::path directory_path = Directory;

   // Check if the directory exists
   if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) {
      printf("Error: Directory '%s' not found or is not a directory\n", Directory);
      return;
   }

   // Iterate through the directory entries
   for (const auto& entry : std::filesystem::directory_iterator(directory_path))
   {
      if (std::filesystem::is_regular_file(entry.status()))
      {
         std::string file_extension = entry.path().extension().string();

         if (file_extension == ".bin")
         {
            TPlaybackFile file;
            bool          found = true;

            file.filename = entry.path().filename().string();

            size_t pos = file.filename.find("_");
            if (pos != std::string::npos)
            {
               file.from_ip = file.filename.substr(pos + 1, file.filename.length() - pos + 1);

               pos = file.from_ip.find("_");
               if (pos != std::string::npos)
               {
                  file.from_mc = file.from_ip.substr(pos + 1, file.from_ip.length() - pos - 5);
                  file.from_ip = file.from_ip.substr(0, pos);
               }
               else
               {
                  found = false;
               }
            }
            else
            {
               found = false;
            }

            if (!found)
               continue;

            files[file.from_ip].emplace_back(file);
         }
      }
   }

   // Give the list of computers to the user, and let them choose one to playback
   printf("\nList of computers to playback:\n");

   std::vector<std::string> file_list;

   for (const auto& file : files)
   {
      file_list.push_back(file.first);
   }

   std::sort(file_list.begin(), file_list.end());

   for (int i = 0; i < file_list.size(); i++)
   {
      printf(" %d) %s\n", i + 1, file_list[i].c_str());
   }

   printf("\nWhich computer to play back:\n");

   int index = 0;

   std::cin >> index;

   if (index)
   {
      index--;
      printf("Playing back computer %s\n", file_list[index].c_str());
   }
   else
   {
      printf("Error: invalid selection\n");
      return;
   }

   // Playback data
   // 1. Create a socket for each multicast address
   // 2. Initialize time to the earliest time out of all the files
   // 3. Playback data until done
   // 4. Loop if needed (or exit)

   std::vector<std::ifstream>   input_files;
   std::vector<CSimUdpSocket*>  sockets;
   std::vector<TPlaybackBuffer> buffers;
   double                       start_time = 0.0;

   const auto& playback_files = files[file_list[index]];

   for (int i = 0; i < playback_files.size(); i++)
   {
      TPlaybackBuffer buffer;
      std::string     filename = Directory;
      filename += "/";
      filename += playback_files[i].filename;
                     
      printf("Opening file %s\n", filename.c_str());
      std::ifstream input_file(filename, std::ios::binary);

      // Read first buffer out of each file
      input_file.read((char*)&buffer.time, sizeof(buffer.time));
      input_file.read((char*)&buffer.bytes, sizeof(buffer.bytes));
      input_file.read(buffer.buffer, buffer.bytes);

      if (buffer.time < start_time || start_time == 0.0)
         start_time = buffer.time;

      input_files.push_back(std::move(input_file));
      buffers.emplace_back(buffer);

      printf("Opening socket %s:%d\n", playback_files[i].from_mc.c_str(), PORT);
      CSimUdpSocket* socket = new CSimUdpSocket();
      socket->Open(playback_files[i].from_mc.c_str(), PORT, PORT);
      socket->SetMultiCast(MY_IP_ADDRESS);
      socket->JoinMcastGroup(playback_files[i].from_mc.c_str(), MY_IP_ADDRESS);
      socket->SetTtl(32);

      sockets.push_back(socket);
   }

   printf("\nStart time %f\n", start_time);

   double next_playback_time = start_time;
   double real_start_time = CSimTimer::GetCurrentTime();

   while (playback_running)
   {
      double curr_time = CSimTimer::GetCurrentTime();
      double delta = curr_time - real_start_time;
      double next_time = start_time + delta;

      // Check if playback is still running
      bool all_zeros = true;
      for (int i = 0; i < buffers.size(); i++)
      {
         if (buffers[i].bytes > 0)
         {
            all_zeros = false;
            break;
         }
      }

      if (all_zeros)
         break;

      // Look over playback buffers and see if it's time to send
      for (int i = 0; i < buffers.size(); i++)
      {
         if (next_time >= buffers[i].time && buffers[i].bytes > 0)
         {
            printf("Sending %d bytes to %s\n", buffers[i].bytes, playback_files[i].from_mc.c_str());
            total_packets_recorded++;
            sockets[i]->SendToSocket(buffers[i].buffer, buffers[i].bytes);

            if (!input_files[i].eof())
            {
               input_files[i].read((char*)&buffers[i].time, sizeof(buffers[i].time));
               input_files[i].read((char*)&buffers[i].bytes, sizeof(buffers[i].bytes));
               input_files[i].read(buffers[i].buffer, buffers[i].bytes);
            }

            if (input_files[i].eof())
            {
               printf("Finished sending data to %s\n", playback_files[i].from_mc.c_str());
               buffers[i].bytes = 0;
            }
         }
      }

      usleep(500);
   }

   printf("%d packets played back\n", total_packets_recorded);
   printf("\nExiting...\n");
}

void record_thread()
{
   CSimUdpSocket socket;
   char          time_str[50] = {};
   in_addr_t     mc_addr_t    = inet_addr(BASE_MC_ADDRESS);
   char          buffer[MAX_BUFFER];
   bool          running = true;

   socket.Open(IP_ADDRESS, PORT, PORT);

   socket.SetMultiCast(MY_IP_ADDRESS);
   for (int i = 0; i < NUM_MC_ADDRESSES; i++)
   {
      in_addr  mc_addr;
      uint32_t offset = (i + 1) << 24;
      char*    mc_address;

      mc_addr.s_addr = mc_addr_t + offset;
      mc_address = inet_ntoa(mc_addr);

      socket.JoinMcastGroup(mc_address, MY_IP_ADDRESS);
   }

   while (running)
   {
      char from_ip[INET_ADDRSTRLEN] = {};
      char from_mc[INET_ADDRSTRLEN] = {};
      int  bytes = 0;

      bytes = socket.ReceiveFromSocket(buffer, MAX_BUFFER, from_ip, from_mc);

      if (bytes > 0)
      {
         CSimTimer::GetCurrentTimeStr(time_str);
         printf("%s: Got message from %s (%s) bytes %d\n", time_str, from_ip, from_mc, bytes);
         total_packets_recorded++;
      }

      thread_data.thread_mutex.lock();

      running = thread_data.running;

      if (bytes > 0)
      {
         thread_data.count++;
         thread_data.data[thread_data.write_index].from_ip = from_ip;
         thread_data.data[thread_data.write_index].from_mc = from_mc;
         thread_data.data[thread_data.write_index].bytes   = bytes;
         thread_data.data[thread_data.write_index].time    = CSimTimer::GetCurrentTime();
         memcpy(thread_data.data[thread_data.write_index].buffer, buffer, bytes);

         thread_data.read_index  = thread_data.write_index;
         thread_data.write_index = 1 - thread_data.write_index;
      }

      thread_data.thread_mutex.unlock();
   }
}

int main(int argc, char* argv[])
{
   char time_str[50] = {};
   bool          record = (argc == 1);

   if (argc > 2)
   {
      printf("Usage: main [playback directory]\n");
      return 1;
   }

   signal(SIGINT, int_handler);

   thread_data.write_index = 0;
   thread_data.read_index = 0;
   thread_data.count = 0;
   thread_data.running = 1;

   if (record)
   {
      std::thread record(record_thread);
      bool        running = true;
      int         prev_count = 0;
      TBuffer     local_buffer;

      // make hash map to store file streams
      std::unordered_map<std::string, std::ofstream> streams;

      CSimTimer::GetCurrentTimeStr(time_str);
      printf("%s: Recording traffic on %s:%d, from .1-.%d\n", time_str, BASE_MC_ADDRESS, PORT, NUM_MC_ADDRESSES);

      while (running)
      {
         thread_data.thread_mutex.lock();

         running = thread_data.running;

         if (thread_data.count != prev_count)
         {
            prev_count           = thread_data.count;
            local_buffer.from_ip = thread_data.data[thread_data.read_index].from_ip;
            local_buffer.from_mc = thread_data.data[thread_data.read_index].from_mc;
            local_buffer.bytes   = thread_data.data[thread_data.read_index].bytes;
            local_buffer.time    = thread_data.data[thread_data.read_index].time;
            memcpy(local_buffer.buffer, thread_data.data[thread_data.read_index].buffer, local_buffer.bytes);
         }

         thread_data.thread_mutex.unlock();

         if (local_buffer.bytes > 0)
         {
            std::string filename = "file_";
            filename += local_buffer.from_ip;
            filename += "_";
            filename += local_buffer.from_mc;
            filename += ".bin";

            if (streams.find(filename) == streams.end())
            {
               // open file
               printf("Opening file %s\n", filename.c_str());
               std::ofstream file(filename, std::ios::binary);
               streams[filename] = std::move(file);
            }

            // write data to file
            streams[filename].write((const char*)&local_buffer.time, sizeof(local_buffer.time));
            streams[filename].write((const char*)&local_buffer.bytes, sizeof(local_buffer.bytes));
            streams[filename].write(local_buffer.buffer, local_buffer.bytes);

            local_buffer.bytes = 0;
         }

         usleep(1000);
      }

      CSimTimer::GetCurrentTimeStr(time_str);
      printf("\n%s: %d packets recorded to %ld files\n", time_str, total_packets_recorded, streams.size());
      printf("Exiting...\n");

      // wait on thread to exit
      // NOTE: Thread blocks on socket read, so just exit without waiting
      // record.join();
   }
   else
   {
      playback(argv[1]);
   }

}
