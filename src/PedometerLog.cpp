/*
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <libusb.h>
#include <stdio.h>
#include <iomanip>
#include <ctime>
#include <stdlib.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

using namespace std;

void dump(char* str, unsigned char *data, int size) {
    int i;
    //  int j;

    qDebug("%s", str);

    for(i=0; i < size; i++ ){
        if ((i & 0x7) == 0 && i >= 8) qDebug("\n");

        //    j=data[i];
        qDebug("%02X ", data[i]);
        //    cout << " " << setfill('0') << setw(2) << hex << j;
    }
}


void transact(libusb_device_handle * dev_handle, unsigned char *data, int tx_len, unsigned char *result, int rx_len, int debug) {
    int r;

    if (debug) {
        dump((char *)"Sending ...:", data, tx_len);
        cout << "    ";
    }

    r = libusb_control_transfer(dev_handle,
                                LIBUSB_REQUEST_TYPE_CLASS |LIBUSB_RECIPIENT_INTERFACE,
                                0x09,
                                0x200,
                                0x0,
                                data,
                                tx_len,
                                5000);

    if(r != 8) {
        qDebug()<<"Write Error "<<r;
        return;
    }

    int actual=0;

    r=libusb_bulk_transfer(dev_handle,
                           0x81,
                           result,
                           rx_len,
                           &actual,
                           2000);

    if (debug) {
        dump((char *)"   returns:", result, 6);
        //    qDebug(" [ %02X %02X]\n", data[6], data[7]);
        //    cout << " [" << setfill('0') << setw(2) << (data[6] + 0x00) << " " << (data[7] + 0x10) << "]" << endl;
    }

    if (r!=0)
        qDebug("Read error %d. Actual read %d\n", r, actual);
}

void packit(unsigned char *data, char* str) {
    /*
  ** Converts a supplied string 'str' into BCD in the array pointed to by data
  ** No checks are made on the supplied string to make sure each digit is numeric.
  */
    unsigned char *p1;
    char *p2;
    unsigned char x;
    int char_count=0;

    p1=data;
    p2=str;
    x=0;

    while (*p2 != 0) {
        x+=*p2++ - '0';
        char_count++;
        if ((char_count & 1) != 0) {
            x=x << 4;
        } else {
            *p1++=x;
            x=0;
        }
    }
}

int unpackit(unsigned char *data, int nibble, int length){
    /*
  ** say data holds 00 07 20 85 01 f2 01 12
  ** nibble = 3, length = 2, returns 72
  ** nibble = 0, length = 3, returns 0
  */

    int result=0;
    int part;
    unsigned char *p1;

    if (length < 0) return 0;

    p1 = data + (nibble / 2);

    while (length--) {
        if ((nibble & 1) != 0) {
            part = *p1 & 0x0F;
            p1++;
        } else {
            part = (*p1 & 0xF0) >> 4;
        }
        result=result*10 + part;
        nibble++;
    }

    return result;
}


bool MainWindow::findDevice(){
    /*
     ** Returns TRUE if the Pedometer is online, else FALSE
     ** Also sets up the correct pointers to the device. If you only
     ** call this routine once, i.e. during program initiation,
     ** then unplug the pedometer, and plug
     ** it in again, something gets screwed up and although the reads
     ** appear to work, the data is a bit iffy.
     ** TODO: Look into why this is.
     */
    libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
    libusb_context *ctx = NULL; //a libusb session
    int r; //for return values
    ssize_t cnt; //holding number of devices in list

    int debug=0;

    r = libusb_init(&ctx); //initialize the library for the session we just declared
    if(r < 0) {
        qDebug()<<"Init Error "<<r; //there was an error
        return false;
    }
    libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

    cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
    if(cnt < 0) {
        qDebug()<<"Get Device Error"; //there was an error
        return false;
    }

    dev_handle = libusb_open_device_with_vid_pid(ctx, 0x0C45, 0x70C1); //these are vendorID and productID I found for my usb device
    libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    if(dev_handle == NULL) {
        //      qDebug()<<"Cannot open device";
        return false;
    }

    if(libusb_kernel_driver_active(dev_handle, 0) == 1) { //find out if kernel driver is attached
        if (debug) qDebug()<<"Kernel Driver Active";

        if(libusb_detach_kernel_driver(dev_handle, 0) == 0) {//detach it
            if (debug) qDebug()<<"Kernel Driver Detached!";
        }
    }

    r = libusb_claim_interface(dev_handle, 0); //claim interface 0

    if(r < 0) {
        qDebug()<<"Cannot Claim Interface";
        return false;
    }
    return true;
}

void MainWindow::download() {
    unsigned char *data = new unsigned char[8]; //data to write
    unsigned char *result = new unsigned char[8];
    unsigned char *capture = new unsigned char[140];

    int debug=0;

    MainWindow::findDevice();
    /*
      ** Start talking to the Pedometer.
      ** it's important to note I have no idea what this data means
      */
    data[0]=0; data[1]=0; data[2]=0; data[3]=0;
    data[4]=0; data[5]=0; data[6]=0xC8; data[7]=0x02;

    transact(dev_handle, data, 8, result, 8, debug);
    /*
      ** Not sure yet what these packets are, but from USB Snoop ...
      ** Appears to be entirely to do with history
      ** If writing to the Pedometer we can ignore this block
      ** of packets
      */

    unsigned char *p1=capture;
    int pedometer_step_length=0;
    int pedometer_weight=0;
    int pedometer_target_steps=0;
    int i;
    bool tflag;      // non-zero means use 'user_target' (number of steps)
    bool sflag;      // non zero means use 'user_step_length'
    bool wflag;      // non-zero means use 'user_weight'
    bool iflag;      // non-zero means 'user_*' fields are imperial
    bool mflag;      // non-zero means 'user_*' fields are metric
    bool unit_swap;  // non-zero means 'user_*' fields are metric, but pedometer is imperial
    //             or 'user_*' fields are imperial, but pedometer is metric
    int user_target, user_weight, user_step;
    char pedometer_is_metric=0;

    tflag=sflag=wflag=iflag=mflag=unit_swap=false;
    user_target=user_weight=user_step=0;

    for (i=0; i<17; i++) {
        data[0]=i*6;
        data[1]=data[2]=data[3]=data[4]=data[5]=0;
        data[6]=i+1;
        data[7]=2;
        transact(dev_handle, data, 8, p1, 8, debug);
        p1+=6;

        if (i==0) {
            /*
          ** Decide if metric or not. I've seen this byte 0x00, 0x01, 0x04 and 0x05.
          ** Cases 1 & 5 were metric
          */
            pedometer_is_metric=capture[4] & 1;

            weight=pedometer_weight=unpackit(capture, 2, 3);
            stepSize=pedometer_step_length=unpackit(capture, 5, 3);
  //          qDebug() << "Step size=" << stepSize;
        } else if (i ==1) {
            /*
          ** The current step target is spread over two USB transactions
          */
            pedometer_target_steps=unpackit(capture, 21, 3) * 100 + unpackit(capture, 0, 2);
        }
    }

    if (pedometer_is_metric) {
        metric=true;
        ui->comboBox_y->setItemText(Y_DISTANCE, "km");
    } else {
        metric=false;
        ui->comboBox_y->setItemText(Y_DISTANCE, "Miles");
    }
    /*
      ** I have no idea what these two packets are, but without them all sorts of
      ** wierd shit happens
      */

    data[0]=0; data[1]=0; data[2]=0; data[3]=0;
    data[4]=0; data[5]=0; data[6]=0; data[7]=0x02;
    transact(dev_handle, data, 8, result, 8, debug);

    data[0]=1; data[1]=0; data[2]=0; data[3]=0;
    data[4]=0; data[5]=0; data[6]=0xC9; data[7]=0x02;
    transact(dev_handle, data, 8, result, 8, debug);

    /*
      ** Generate a packet with weight and step length.
      ** These are initially set by reading the current values from the pedometer.
      ** Normally we ripple through the imperial/metric state of the pedometer,
      ** but if we are changing units, then alter the user weight/step length too
      */

    char buffer[100];
    int new_mode;
    int new_weight, new_pedometer_step_length;
    new_weight=pedometer_weight;
    new_pedometer_step_length=pedometer_step_length;

    new_mode=(pedometer_is_metric ? 5 : 0);
    if (iflag)
        new_mode=0;
    if (mflag)
        new_mode=5;

    if (unit_swap) {
        if (pedometer_is_metric) {
            new_weight=pedometer_weight*2.20462;
            new_pedometer_step_length=pedometer_step_length*0.3937;
        } else {
            new_weight=pedometer_weight*0.45359;
            new_pedometer_step_length=pedometer_step_length*2.54;
        }
        //    fprintf(stderr,"New weight = %d New step length=%d\n", new_weight, new_pedometer_step_length);
    }

    sprintf(buffer,"0000%03d%03d%02d0103", (wflag?user_weight:new_weight), (sflag?user_step:new_pedometer_step_length), new_mode);
    packit(data, buffer);
    transact(dev_handle, data, 8, result, 8, debug);

    data[0]=4; data[1]=0; data[2]=0; data[3]=0;
    data[4]=0; data[5]=0; data[6]=0xC9; data[7]=0x02;
    transact(dev_handle, data, 8, result, 8, debug);

    /*
      ** Generate a packet with Time of Day and our target no of steps
      ** format 0hhmmnnnnn070103
      ** where hh = hours. Clock is always 24hrs. Pedometer displays time as 12hr if requested.
      **       mm = mins
      **       nnnnn = target number of steps
      ** Stuff I'm unsure about ...
      ** The leading zero, and the trailing "070103"
      */

    time_t now = time(0);
    tm *ltm = localtime(&now);
    int target=pedometer_target_steps;

    new_mode=(pedometer_is_metric ? 7 : 15);
    if (iflag)
        new_mode=15;
    if (mflag)
        new_mode=7;

    sprintf(buffer, "0%02d%02d%05d%02d0103", ltm->tm_hour, ltm->tm_min, (tflag?user_target:pedometer_target_steps),new_mode);
    packit(data, buffer);

    transact(dev_handle, data, 8, result, 8, debug);

    int steps=0;
    int step_target=target;
    int distance=0;
    int calories=0;
    int walking_time=0;

    int weight_in_ped=unpackit(capture, 2, 3);
    int step_distance=unpackit(capture, 5, 3);
    int added, updated;
 //   qDebug() << "pedometer_weight=" << pedometer_weight << "weight_in_ped="<< weight_in_ped;
    added=updated=0;
    for (i=0; i<8; i++) {
        steps=0;

        if (i == 0) {
            steps=unpackit(capture, 12, 4) + (unpackit(capture, 35, 1) * 10000);
            distance=unpackit(capture, 25, 5);
            calories=unpackit(capture, 30, 5);
            walking_time=(unpackit(capture, 44, 4) * 10) + unpackit(capture, 24, 1);
        } else if (i == 1) {
            steps=unpackit(capture, 67, 5);
            distance=unpackit(capture, 81, 3) * 100 + unpackit(capture, 60, 2);
            calories=unpackit(capture, 62, 5);
            walking_time=unpackit(capture, 76, 5);
        } else if (i == 2) {
            steps=unpackit(capture, 72, 4) + (unpackit(capture, 95, 1) * 10000);
            distance=unpackit(capture, 85, 5);
            calories=unpackit(capture, 90, 5);
            walking_time=(unpackit(capture, 104, 4) * 10) + unpackit(capture, 84, 1);
        } else if (i == 3) {
            steps=unpackit(capture, 99, 5);
            distance=unpackit(capture, 113, 5);
            calories=unpackit(capture, 118, 2) * 1000 + unpackit(capture, 96, 3);
            walking_time=unpackit(capture, 108, 5);
        } else if (i == 4) {
            steps=unpackit(capture, 127, 5);
            distance=unpackit(capture, 141, 3) * 100 + unpackit(capture, 120, 2);
            calories=unpackit(capture, 122, 5);
            walking_time=unpackit(capture, 136, 5);
        } else if (i == 5) {
            steps=unpackit(capture, 132, 4) + unpackit(capture, 155, 1) * 10000;
            distance=unpackit(capture, 145, 5);
            calories=unpackit(capture, 150, 5);
            walking_time=unpackit(capture, 164, 4) * 10 + unpackit(capture, 144, 1);
        } else if (i == 6) {
            steps=unpackit(capture, 159, 5);
            distance=unpackit(capture, 173, 5);
            calories=unpackit(capture, 178, 2) * 1000 + unpackit(capture, 156, 3);
            walking_time=unpackit(capture, 168, 5);
        } else if (i == 7) {
            steps=unpackit(capture, 187, 5);
            distance=unpackit(capture, 201, 3) * 100 + unpackit(capture, 180, 2);
            calories=unpackit(capture, 182, 5);
            walking_time=unpackit(capture, 196, 5);
        }

        sprintf(buffer, "%04d-%02d-%02d", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday);

        now -=86400;
        ltm = localtime(&now);

        if (steps != 0)
            insertUpdate(buffer, steps, step_target, step_distance, weight_in_ped,
                         (float)distance/100.0, (float)calories/10.0, walking_time, added, updated);
    }

    delete[] data;
    delete[] result;
    delete[] capture;

    plotGraph();

    updStatus(added, updated);
}

