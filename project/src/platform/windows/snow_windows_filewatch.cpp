/*
    Copyright Sven Bergström, Micheal Bickel 2014
    created for snow https://github.com/underscorediscovery/snow
    MIT license
*/

#include "snow_core.h"
#include "snow_io.h"
#include "common/snow_event_queue.h"


#include <string>
#include <vector>

#include <windows.h>

namespace snow {

    extern double timestamp();

    namespace io {

//:todo : Testing, this may change names soon
#ifdef SNOW_NO_FILE_WATCH

        std::vector<std::string> watched_paths;
        bool init_filewatch(){ return false; }
        void shutdown_filewatch(){}
        void update_filewatch(){}
        void refresh_filewatch(){}

#else

        static eventqueue_t filewatch_queue;

        class FileWatcherThread {

          public:
            bool running;
            std::string path;
            HANDLE handle;
            HANDLE thread;
            OVERLAPPED overlap;
            BYTE* buffer;
            size_t buffer_size;
            CRITICAL_SECTION critical;

            static DWORD WINAPI run_thread( void *watcher );

            FileWatcherThread(const std::string &_path) {

                running = false;
                path = std::string(_path);
                buffer_size = 1024;
                buffer = new BYTE[buffer_size];

                    //I spitballed this 2048 spin count from reading some posts online
                int _spincount = 2048;
                #ifndef DEBUG
                    ::InitializeCriticalSectionEx(&critical, _spincount, CRITICAL_SECTION_NO_DEBUG_INFO);
                #else
                    ::InitializeCriticalSectionEx(&critical, _spincount, 0);
                #endif

                handle = ::CreateFile(
                    path.c_str(),
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                    NULL
                );

                if(handle != INVALID_HANDLE_VALUE) {

                    ZeroMemory(&overlap, sizeof(overlap));
                    overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

                    DWORD threadid;
                    running = true;
                    thread = CreateThread(NULL, 0, &run_thread, this, 0, &threadid);

                } //!invalid handle

            } //construct

            ~FileWatcherThread() {

                running = false;

                ::LeaveCriticalSection(&critical);
                ::DeleteCriticalSection(&critical);

                ::CancelIo(handle);
                ::CloseHandle(handle);
                ::CloseHandle(thread);

            } //~

        }; //class FileWatcherThread


        DWORD WINAPI FileWatcherThread::run_thread( void *_watcher ) {

            FileWatcherThread *watcher = (FileWatcherThread*)_watcher;


            while(watcher->running) {

                ReadDirectoryChangesW(
                    watcher->handle, watcher->buffer, watcher->buffer_size, TRUE,
                    FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                    NULL, &watcher->overlap, NULL
                );

                WaitForSingleObject(watcher->overlap.hEvent, INFINITE);

                    int seek = 0;

                    while(seek < watcher->buffer_size) {

                        PFILE_NOTIFY_INFORMATION notifier = PFILE_NOTIFY_INFORMATION(watcher->buffer + seek);

                        WCHAR szwFileName[MAX_PATH];
                        int ulCount = notifier->FileNameLength/2;
                        wcsncpy(szwFileName, notifier->FileName, ulCount);
                        szwFileName[ulCount] = L'\0';

                        std::wstring widepath(szwFileName);
                        std::string path(widepath.begin(), widepath.end());

                        FileEventType _event_type = fe_unknown;

                            switch( notifier->Action ) {

                                case FILE_ACTION_ADDED:{
                                    _event_type = fe_create;
                                    break;
                                }

                                case FILE_ACTION_REMOVED:{
                                    _event_type = fe_remove;
                                    break;
                                }

                                case FILE_ACTION_MODIFIED:{
                                    _event_type = fe_modify;
                                    break;
                                }

                                case FILE_ACTION_RENAMED_OLD_NAME:{
                                    _event_type = fe_remove;
                                    break;
                                }

                                case FILE_ACTION_RENAMED_NEW_NAME:{
                                    _event_type = fe_create;
                                    break;
                                }

                            } //switch

                            if(_event_type != fe_unknown) {

                                // put into queue
                                event_node_t* node = new event_node_t;
                                node->event = new FileEvent( _event_type, snow::timestamp(), (watcher->path+"/"+path) );
                                eventqueue_push(&filewatch_queue, node);

                            }

                        seek += notifier->NextEntryOffset;

                        if(notifier->NextEntryOffset == 0) {
                            break;
                        }

                    } //seek < buffer size

                } //while running

            return 0;

        } //run_thread

        std::vector<std::string> watched_paths;
        std::vector<FileWatcherThread*> watchers;

        bool init_filewatch() {

            snow::log(2, "/ snow / initialized file watch ok");

            // create our queue
            eventqueue_create((eventqueue_t*)&filewatch_queue);

            return true;

        } //init_filewatch

        void start_filewatch(){

            if(watched_paths.size() == 0) {
                return;
            }

            std::vector<std::string>::iterator it = watched_paths.begin();

            for( ; it != watched_paths.end(); ++it) {

                std::string path = *it;

                FileWatcherThread* watcher = new FileWatcherThread(path);
                watchers.push_back(watcher);

            } //each watched path

            snow::log(2, "/ snow / filewatch started");

        } //start_filewatch

        void stop_filewatch() {

            if(watchers.size() == 0) {
                return;
            }

            std::vector<FileWatcherThread*>::iterator it = watchers.begin();

            for( ; it != watchers.end(); ++it) {
                FileWatcherThread* watcher = *it;
                if(watcher) {
                    delete watcher;
                }
            }

            watchers.clear();

            snow::log(2, "/ snow / filewatch stopped");

        } //stop_filewatch

        void refresh_filewatch() {

            stop_filewatch();
            start_filewatch();

        } //refresh_filewatch

        void update_filewatch() {

            // clear the queue
            event_node_t* node;
            while( node = eventqueue_pop(&filewatch_queue) ) {
                snow::io::dispatch_event( *node->event );
                delete node->event;
                delete node;
            }

        } //update_filewatch

        void shutdown_filewatch() {

            stop_filewatch();

            snow::log(2, "/ snow / filewatch shutdown");

        } //shutdown_filewatch

#endif //SNOW_NO_FILE_WATCH

    } //io namespace

} //snow namespace

