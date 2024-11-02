#include "driver.hpp"
#include <iostream>
#include <unistd.h>
#include "libevdipp/debug.hpp"



ScreenDriver::ScreenDriver(const Evdi& evdi, std::vector<unsigned char>& edid, bool separateCursorEvents)
: Screen(evdi, edid, separateCursorEvents), myevdi(evdi)
{
    //usbservice = new UsbDisplay();

}

void ScreenDriver::add_screen(ScreenPtr screen, libusb_device *dev)
{       
    usbservice = new UsbDisplay();
    edid_data.clear();
   
   // Edid_data=usbservice->getEdid();
    //for(int i =0; i<256 ;i++)
    //{
       // 
        //printf("%d : 0x%02x\n",i,Edid_data[i]);
    //    edid_data.push_back(Edid_data[i]);
  //  }
   // printf("edid vector size : %ld\n",edid_data.size());
    //setEdid(edid_data);
    usbservice->jxInit(dev);
    
    screens.push_back(screen);
    auto watcher = std::make_shared<ev::io>();
    watcher->set<ScreenDriver, &ScreenDriver::on_screen_event>(this);
    watcher->data = screen.get();
    watcher->start(screen->event_source(), ev::READ);
    screen_event_watchers.push_back(watcher);
}

void ScreenDriver::on_dpms_notification(int dpms_mode){
    logging << "screenDriver DPMS event: " << dpms_mode << std::endl;
    
    if(dpms_mode == 3) // display turn off
        usbservice->jxSetResolution(0,0);
    
}

int ScreenDriver::on_update_ready(int buffer_to_be_updated)
{

    int num_rects;
    myevdi.grab_pixels(rects.data(), &num_rects);

    evdi_buffer buffer = buffers[bufferToUpdate]->get();
    //void* buffer->buffer;
    //combineCursorBuffer(buffer);
    usbservice->jxStartEncode((char *)buffer.buffer,buffer.width ,buffer.height,buffer.width);


    //logging << "Update ready for buffer " << buffer_to_be_updated << std::endl;
    if (num_rects > 1) {
        // Do not bother logging single dirty rects, only log more interesting cases
        logging << "ScreenDriver --- Have " << num_rects << " to redraw" << std::endl;

        for (size_t i = 0; i < num_rects; ++i) {
            logging << "[" << rects[i].x1 << "," << rects[i].y1 << ":" << rects[i].x2 << "," << rects[i].y2 << "] ";
        }
        logging << std::endl;
    }
    return num_rects;
}

void ScreenDriver::on_cursor_set(evdi_cursor_set cursor_set)
{
    logging << "ScreenDriver --- Cursor " << (cursor_set.enabled ? "ON" : "OFF") << ": Hot[X,Y]=" << cursor_set.hot_x << "," << cursor_set.hot_y << std::endl;
}

void ScreenDriver::on_cursor_move(evdi_cursor_move cursor_move)
{
    cursor_position = cursor_move;
    logging << "ScreenDriver --- Cursor moved to " << cursor_move.x << "," << cursor_move.y << std::endl;
}


void ScreenDriver::quit()
{
    std::cout << "Qutting..." << std::endl;
    usbservice->jxDelete();
    delete usbservice;
    screens.front().reset();
    ev::get_default_loop().break_loop(ev::ALL);
}

void ScreenDriver::on_screen_event(ev::io &w, int revents)
{
    static_cast<Screen*>(w.data)->handle_events();
}

void ScreenDriver::on_enter_pressed(ev::io &w, int revents)
{
    quit();
}

void ScreenDriver::on_sigint(ev::sig &w, int revents)
{
    quit();
}

void ScreenDriver::on_sync(ev::periodic &w, int revents)
{
    
    for (const auto& screen : screens) {
        screen->update();
    }

    timeval tv = {0, 0};

    int rc = libusb_handle_events_timeout (NULL,&tv);
    if (rc < 0)printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));
}

void ScreenDriver::drive(unsigned fps)
{
    ev::default_loop loop;

    /*stdin_watcher.set<ScreenDriver, &ScreenDriver::on_enter_pressed>(this);
    stdin_watcher.start(STDIN_FILENO, ev::READ);*/

    const double update_interval = 1.0 / fps;
    sync_timer.set<ScreenDriver, &ScreenDriver::on_sync>(this);
    sync_timer.start(0, update_interval);

    /*
    ev::sig sigint_handler;
    sigint_handler.set<ScreenDriver, &ScreenDriver::on_sigint>(this);
    sigint_handler.start(SIGINT);

    std::cout << "Press Enter or Ctrl+C to quit..." << std::endl;*/

    loop.run(0);
}

void ScreenDriver::combineCursorBuffer(evdi_buffer buffer){
    unsigned int *data = (unsigned int *)buffer.buffer;
    
    int pwidth = buffer.width;
    int phight = buffer.height;
    for(int i=0;i<292-277+1;i++)
        for(int j=0;j<345-336+1;j++)
        {
            if( !((Cursor_image [(345-336+1)*4*i + j*4] == 0)&&(Cursor_image [(345-336+1)*4*i + j*4+2] == 0)&&(Cursor_image [(345-336+1)*4*i + j*4+1] == 255))){
                int x = cursor_position.x + j-3;
                int y = cursor_position.y + i-4;
                if(x>=0 &&  x<pwidth && y>=0  &&  y<phight )
                {//printf("Mouse is at (%d,%d)\n", x, y);
                    data [pwidth * y + x] = (data [pwidth * y + x] &0xffffff00)|(long unsigned int)Cursor_image [(345-336+1)*4*i + j*4];
                    data [pwidth * y + x] = (data [pwidth * y + x] &0xffff00ff)|((long unsigned int)Cursor_image [(345-336+1)*4*i + j*4+1]<<8);
                    data [pwidth * y + x] = (data [pwidth * y + x] &0xff00ffff)|((long unsigned int)Cursor_image [(345-336+1)*4*i + j*4+2]<<16);
                    data [pwidth * y + x] = (data [pwidth * y + x] &0x00ffffff)|((long unsigned int)Cursor_image [(345-336+1)*4*i + j*4+3]<<24);
                }
            }
        }
}
