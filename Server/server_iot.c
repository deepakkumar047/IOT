#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define MAX 16
#define PORT 4321
#define SA struct sockaddr
#define REF_TIME 1900
#define TM_DIFF 10 //in Seconds
#define TMOUT 8
#define K 5
#define MAX_INST 50
#define SUNNY 0
#define RAINY 1
#define MAX_SZ 128
#define STD_AIR_QUALITY 1000

FILE *fp;

typedef struct {
	double d;  //square of euclidean distance
	int x;	//index in array
} eds;

int comparator(const void *p, const void *q)	//sorting in descending order
{ 
	if((((eds *)p)->d) > (((eds *)q)->d))
		return 1;
	else if((((eds *)p)->d) < (((eds *)q)->d))
		return -1;
	else
		return 0;
}

double eucd_dist_square(double x_1, double y_1, double x_2, double y_2)
{
	return ((x_1 - x_2) * (x_1 - x_2)) + ((y_1 - y_2) * (y_1 - y_2));
}

int knn_classifier(double humd, double temp, double inst[][2], int class[], int size)
{
	eds dist_arr[MAX_INST];
	int i;
	for(i = 0; i < size; i++)
	{
		//printf("%.2lf, %.2lf, %d\n", inst[i][0], inst[i][1], class[i]);
		dist_arr[i].d = eucd_dist_square(humd, temp, inst[i][0], inst[i][1]);
		dist_arr[i].x = i;
	}
	
	qsort(dist_arr, size, sizeof(eds), comparator);
	/*
	for(i = 0; i < size; i++)
	{
		printf("%d, %lf\n", dist_arr[i].x, dist_arr[i].d);
	}
	printf("\n");
	*/
	if(K > size)
	{
		printf("ERROR: K is greater than Training Data Set\n");
		return -1;
	}
	int count_S = 0, count_R = 0;
	for(i = 0; i < K; i++)
	{
		int x = dist_arr[i].x;
		if(class[x] == 0)
			count_S++;
		else
			count_R++;
	}
	//printf("count_S = %d, count_R = %d\n", count_S, count_R);
	if(count_S > count_R)
		return SUNNY;
	else
		return RAINY;
}

void cluster(double humd, double temp, double aq)
{
	FILE *fp_data_set, *fp_out;
	char *fname_data_set = "TRAINING_DATA_WEATHER.csv";
	char *fname_out = "CLASSIFIED_DATA.csv";
	char data[MAX_SZ];

	fp_data_set = fopen(fname_data_set, "r");
	if(fp_data_set == NULL)
	{
		printf("Error: 'TRAINING_DATA_WEATHER.csv'\n");
		exit(0);
	}
	
	fp_out = fopen(fname_out, "a");
	if(fp_out == NULL)
	{
		printf("Error: 'CLASSIFIED_DATA.csv'\n");
		exit(0);
	}

	double inst[MAX_INST][2];
	int class[MAX_INST], i = 0;
	char class_str[16];
	while(fgets(data, MAX_SZ, fp_data_set))	//Reading from Training Data set
	{
		//printf("%s", data);
		sscanf(data, "%lf,%lf,%s", &inst[i][0], & inst[i][1], class_str);
		//printf("%.2lf, %.2lf, %s\n", inst[i][0], inst[i][1], class_str);
		if(!strcmp(class_str, "Sunny"))
		{
			//printf("Sunny\n");
			class[i] = 0;
		}
		else	//Rainy
		{
			//printf("Rainy\n");
			class[i] = 1;
		}
		i++;
	}
	int size = i;

	int cl = knn_classifier(humd, temp, inst, class, size);
	char air_ql[10] = " Good";
	if(aq > STD_AIR_QUALITY)
		strcpy(air_ql, " Bad ");;
	if(cl == SUNNY)
	{
		printf("|   %6.2lf    |  %6.2lf  | %7.2lf | Sunny | %s |\n", humd, temp, aq, air_ql);
		fprintf(fp_out, "%.2lf,%.2lf,Sunny\n", humd, temp);
	}
	else if(cl == RAINY)
	{
		printf("|   %6.2lf    |  %6.2lf  | %7.2lf | Rainy | %s |\n", humd, temp, aq, air_ql);
		fprintf(fp_out, "%.2lf,%.2lf,Rainy\n", humd, temp);
	}
	else	//cl = -1
		printf("ERROR: -1\n");
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fclose(fp_data_set);
	fclose(fp_out);
}

void getData(int sockfd) 
{ 
	char buff[MAX], fname[20] = "SENSOR_DATA.csv";
	float h, t, ppm;
	int sz;
	bool flag = true;
	printf("\n\nNodeMCU[ESP-8266] Sensor Data:\n\n");
	fd_set set;
	struct timeval timeout;
	int rv;
	FD_ZERO(&set); /* clear the set */
	FD_SET(sockfd, &set);
	while(1)
	{
		//Humidity
		timeout.tv_sec = TMOUT;	//wait for 8 sec to read from the socket
		timeout.tv_usec = 0;
		rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
		{
			printf("Error: select() [-1]\n");
			return;
		}
		else if(rv == 0)
		{
			printf("\nTimeout !! [H]\n");
			return;
		}
		else
		{
	        	bzero(buff, MAX);
			sz = read(sockfd, buff, sizeof(buff));
			if(sz == -1)
			{
				printf("Error: read()!! [H]\n");
				return;
			}
			sscanf(buff, "%f", &h);
		}

		//Temperature
		timeout.tv_sec = TMOUT;
		timeout.tv_usec = 0;
		rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
		{
			printf("Error: select() [-1]\n");
			return;
		}
		else if(rv == 0)
		{
			printf("\nTimeout ! [T]!\n");
			return;
		}
		else
		{
			bzero(buff, MAX);
			sz = read(sockfd, buff, sizeof(buff));
			if(sz == -1)
			{
				printf("Error: read()!! [T]\n");
				return;
			}
			sscanf(buff, "%f", &t);
		}
	
		//Air Quality
		timeout.tv_sec = TMOUT;
		timeout.tv_usec = 0;
		rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
		if(rv == -1)
		{
			printf("Error: select() [-1]\n");
			return;
		}
		else if(rv == 0)
		{
			printf("\nTimeout !! [AQ]\n");
			return;
		}
		else
		{
			bzero(buff, MAX);
			sz = read(sockfd, buff, sizeof(buff));
			if(sz == -1)
			{
				printf("Error: read()!! [AQ]\n");
				return;
			}
			sscanf(buff, "%f", &ppm);
		}
		//printf("Humidity: %.2f %%     Temperature: %.2f °C     Air Quality: %.2f ppm\n", h, t, ppm);

		//Timestamp
		time_t current_time = time(NULL);
		struct tm *local_time;
		local_time = localtime(&current_time);

		int hh = local_time->tm_hour;
		int mm = local_time->tm_min;
		int ss = local_time->tm_sec;
		int yyyy = REF_TIME + local_time->tm_year;
		int month = local_time->tm_mon + 1;
		int month_day = local_time->tm_mday;
		
		time_t time_new, time_old;
		time_new = current_time;

		if(flag)
		{
			fp = fopen(fname, "a");
			if(fp == NULL)
			{
				printf("File couldn't be opened!\n");
				exit(1);
			}
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			printf("| Humidity(%%) | Temp(°C) | AQ(ppm) | Class |  AQ   |\n");
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			cluster(h, t, ppm);	//Clustering the Data
			//Humidity(%), Temperature(°C), Air Quality(PPM), YYYY, MONTH, MONTH DAY, HH, MM, SS
			fprintf(fp, "%.2f,%.2f,%.2f,%04d:%02d:%02d:%02d:%02d:%02d\n", h, t, ppm, yyyy, month, month_day, hh, mm, ss);
			fclose(fp);
			time_old = time_new;
			flag = false;
		}
		
		double time_diff = difftime(time_new, time_old);
		//printf("time_diff = %lf\n", time_diff);
		if(time_diff >= TM_DIFF)
		{
			fp = fopen(fname, "a");
			if(fp == NULL)
			{
				printf("File couldn't be opened!\n");
				exit(1);
			}
			cluster(h, t, ppm);	//Clustering the Data
			//Humidity(%), Temperature(°C), Air Quality(PPM), YYYY, MONTH, MONTH DAY, HH, MM, SS
			fprintf(fp, "%.2f,%.2f,%.2f,%04d:%02d:%02d:%02d:%02d:%02d\n", h, t, ppm, yyyy, month, month_day, hh, mm, ss);			time_old = time_new;
			fclose(fp);
		}
	}
} 

int main() 
{ 
	int sockfd, connfd, opt = 1;
	unsigned int len; 
	struct sockaddr_in servaddr, cli; 
  
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
	{ 
		printf("Socket creation failed...\n"); 
	        exit(0); 
	} 
    	//else
        //	printf("Socket successfully created..\n");

	// Forcefully attaching socket to the port 4321
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
	{ 
		perror("setsockopt");
		printf("Error: setsockopt!\n");
		exit(EXIT_FAILURE); 
	}
	bzero(&servaddr, sizeof(servaddr)); 
		  
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 
	  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) 
	{ 
		printf("Socket bind failed...\n"); 
		exit(0); 
	} 
	//else
	//	printf("Socket successfully binded..\n"); 
	  
	// Now server is ready to listen and verification
	if ((listen(sockfd, 1)) != 0)
	{ 
		printf("Listen failed...\n"); 
		exit(0); 
	}
	
	len = sizeof(cli); 
		  
	while(1)
	{
		printf("\nWaiting for NodeMCU[ESP-8266]...\n");
		// Accept the data packet from client and verification 
		connfd = accept(sockfd, (SA*)&cli, &len); 
		if (connfd < 0)
		{ 
			printf("Server accept failed...\n"); 
			exit(0); 
		} 
		else
			printf("Connected to NodeMCU[ESP-8266]\n");
		getData(connfd);
		close(connfd);
	}
	close(sockfd);
	return 0;
}
