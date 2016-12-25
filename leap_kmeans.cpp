#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <vector>
#include <iterator>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <cmath>

using namespace std;
#define K_COUNT 9
#define DATA_COUNT 25000

class CSVRow
{
public:
   std::string const& operator[](std::size_t index) const
   {
      return m_data[index];
   }
   std::size_t size() const
   {
      return m_data.size();
   }
   void readNextRow(std::istream& str)
   {
      std::string line;
      std::getline(str, line);

      std::stringstream lineStream(line);
      std::string cell;

      m_data.clear();
      while (std::getline(lineStream, cell, ','))
      {
         m_data.push_back(cell);
      }
      //This checks for a trailing comma with no data after it.
      if (!lineStream && cell.empty())
      {
         m_data.push_back("");
      }
   }
private:
   std::vector<std::string> m_data;
};

std::istream& operator>>(std::istream& str, CSVRow& data)
{
   data.readNextRow(str);
   return str;
}

class pos
{
public:
   double pitch;
   double roll;
   double yaw;
   int position;
};

int main()
{
   pos* k = new pos[K_COUNT];
   pos* center = new pos[K_COUNT];
   pos* manualCenter = new pos[K_COUNT];
   manualCenter[0].pitch = 20.0; manualCenter[0].roll = -10.0; manualCenter[0].yaw = 7.0;
   manualCenter[1].pitch = 9.0; manualCenter[1].roll = 25.0; manualCenter[1].yaw = 3.0;
   manualCenter[2].pitch = -26.0; manualCenter[2].roll = 11.0; manualCenter[2].yaw = 28.0;
   manualCenter[3].pitch = -39.0; manualCenter[3].roll = -15.0; manualCenter[3].yaw = 15.0;
   manualCenter[4].pitch = -50.0; manualCenter[4].roll = -80.0; manualCenter[4].yaw = -30.0;
   manualCenter[5].pitch = -3.0; manualCenter[5].roll = -90.0; manualCenter[5].yaw = 15.0;
   manualCenter[6].pitch = 50.0; manualCenter[6].roll = -70.0; manualCenter[6].yaw = 40.0;
   manualCenter[7].pitch = 70.0; manualCenter[7].roll = -30.0; manualCenter[7].yaw = 0.0;
   manualCenter[8].pitch = 45.0; manualCenter[8].roll = 18.0; manualCenter[8].yaw = -15.0;
   for (int i = 0; i < K_COUNT; i++)
   {
      manualCenter[i].pitch = manualCenter[i].pitch / 100;
      manualCenter[i].roll = manualCenter[i].roll / 100;
      manualCenter[i].yaw = manualCenter[i].yaw / 100;
   }
   double count_Group[K_COUNT] = { 0, };
   
   vector< pos > datas;
   vector< double > distance[K_COUNT];
   double saveLable[K_COUNT][3];

   //data insert
   std::ifstream file("example.csv");
   CSVRow row;
   while (file >> row)
   {
      pos tmp;
      tmp.pitch = stod(row[0]);
      tmp.roll = stod(row[1]) * 2;
      tmp.yaw = stod(row[2])* 5;
      if (tmp.pitch > 200.0 && tmp.pitch < -200.0 && tmp.roll > 200.0 && tmp.roll < -200.0 && tmp.yaw >200.0 
&& tmp.yaw < -200.0)//정상범위를 벗어난 값이면 처리하지 않는다.
      {
         continue;
      }
      datas.push_back(tmp);

   }

   //random k , init
   for (int i = 0; i < K_COUNT; i++)
   {
      k[i] = datas[i];
      center[i].pitch = datas[i].pitch;
      center[i].roll = datas[i].roll;
      center[i].yaw = datas[i].yaw;
      distance[i].resize(DATA_COUNT);
   }
   bool loop = true;
   while (loop)
   {//when the k-positions are all same with next position.
      //center init
      for (int i = 0; i < K_COUNT; i++)
      {
         count_Group[i] = 0;
      }
      //distance
      for (int i = 0; i < datas.size(); i++)
      {
         for (int j = 0; j < K_COUNT; j++)
         {
            double tmp_distance = sqrt(pow(k[j].pitch - datas[i].pitch, 2) +
               pow(k[j].roll - datas[i].roll, 2) +
               pow(k[j].yaw - datas[i].yaw, 2));
            distance[j][i] = tmp_distance;
         }
      }
      //get center
      for (int i = 0; i < datas.size(); i++)
      {
         double min = distance[0][i];
         int min_j = 0;

         for (int j = 1; j < K_COUNT; j++)
         {
            if (min > distance[j][i])
            {
               min = distance[j][i];
               min_j = j;
            }
         }
         center[min_j].pitch += (datas[i].pitch - center[min_j].pitch) / datas.size();
         center[min_j].roll += (datas[i].roll - center[min_j].roll) / datas.size();
         center[min_j].yaw += (datas[i].yaw - center[min_j].yaw) / datas.size();
         count_Group[min_j]++;
      }
      //change K
      int same_count = 0;
      for (int i = 0; i < K_COUNT; i++)
      {
         if (count_Group[i] != 0)
         {
            if ((center[i].pitch == k[i].pitch)
               && (center[i].roll == k[i].roll)
               && (center[i].yaw  == k[i].yaw))
            {
               same_count++;
               
            }
            else
            {
               k[i].pitch = center[i].pitch;
               k[i].roll = center[i].roll;
               k[i].yaw = center[i].yaw;
            }
         }
         
         if (same_count == K_COUNT)
         {
            loop = false;
         }
         cout << fixed << setprecision(2);
         
      }
      
   }//end of loop
   double distanceBuffer[K_COUNT];
   //lable the group
   for (int i = 0  ; i < K_COUNT; i++)
   {
      double distanceMin = 0.0;
      for (int j = 0; j < K_COUNT; j++)
      {
         distanceBuffer[j] = sqrt(pow(center[i].pitch - manualCenter[j].pitch, 2) +
            pow(center[i].roll - manualCenter[j].roll, 2)
            + pow(center[i].yaw - manualCenter[j].yaw, 2));
      }
      for (int j = 1, distanceMin = distanceBuffer[0]; j < K_COUNT; j++)
      {
         if (distanceMin > distanceBuffer[j])
         {
            distanceMin = distanceBuffer[j];
            center[i].position = j;
         }
      }
      
   }
   for (int i = 0; i < K_COUNT; i++)
   {
      printf("%d\n", center[i].position);
   }
   std::ofstream myfile;
   myfile.open("center.csv");
   for (int i = 0; i < K_COUNT; i++)
   {
      myfile << center[i].roll << "," << center[i].pitch << "," << center[i].yaw << std::endl;

   }
   myfile.close();

}