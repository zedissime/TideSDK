/**
 * Copyright (c) 2012 - 2014 TideSDK contributors 
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include "tide.h"
#include "thread_manager.h"
#include <Poco/Bugcheck.h>

namespace tide
{
    AsyncJob::AsyncJob(TiMethodRef job) :
        StaticBoundObject(),
        job(job),
        completed(false),
        result(Value::Undefined),
        hadError(false),
        cancelled(false),
        thread(0),
        adapter(0)
    {
        this->SetProgress(0.0);
        this->SetMethod("getProgress", &AsyncJob::_GetProgress);
        this->SetMethod("cancel", &AsyncJob::_Cancel);
        this->SetMethod("isComplete", &AsyncJob::_IsComplete);
    }

    AsyncJob::~AsyncJob()
    {
        this->progressCallbacks.clear();
        this->completedCallbacks.clear();
        this->errorCallbacks.clear();
        this->job = NULL;
    }

    void AsyncJob::RunAsynchronously()
    {
        this->adapter = new Poco::RunnableAdapter<AsyncJob>(*this,
            &AsyncJob::RunThreadTarget);
        this->thread = new Poco::Thread();
        this->thread->start(*this->adapter);
    }

    void AsyncJob::RunThreadTarget()
    {
        // We are now in a new thread -- on OSX we need to do some 
        // basic bookkeeping for the reference counter, but other
        // than that, everything past here is like executing a job
        // in a synchronous fashion.
        START_TIDE_THREAD;
        this->Run();

        try
        {
            delete this->adapter;
        }
        catch (Poco::BugcheckException& bge)
        {
            std::cerr << bge.what() << std::endl;
        }

        END_TIDE_THREAD;
    }

    void AsyncJob::Run()
    {
        // Make sure this job sticks around at least until it finishes.
        TiObjectRef save(this, true);

        this->result = this->Execute();
        if (!this->hadError)
        {
            this->completed = true;
            this->OnCompleted();

            std::vector<TiMethodRef>::iterator i = this->completedCallbacks.begin();
            while (i != this->completedCallbacks.end())
            {
                this->DoCallback(*i++, true);
            }
        }
    }

    ValueRef AsyncJob::Execute()
    {
        try
        {
            return this->job->Call(this->arguments);
        }
        catch (ValueException& e)
        {
            this->Error(e);
            return Value::Undefined;
        }
    }

    void AsyncJob::Cancel()
    {
        this->cancelled = true;
    }

    double AsyncJob::GetProgress()
    {
        return this->progress;
    }

    void AsyncJob::DoCallback(TiMethodRef method, bool reportErrors)
    {
        ValueList args(Value::NewObject(GetAutoPtr()));
        RunOnMainThread(method, args, false);
    }

    void AsyncJob::SetProgress(double progress, bool callbacks)
    {
        if (progress < 0.0)
        {
            progress = 0.0;
        }
        else if (progress > 1.0)
        {
            progress = 1.0;
        }
        this->progress = progress;

        /*
         * One can avoid an infinite loop by setting callbacks=false
         */
        if (callbacks)
        {
            this->OnProgressChanged();

            std::vector<TiMethodRef>::iterator i = this->progressCallbacks.begin();
            while (i != this->progressCallbacks.end())
            {
                this->DoCallback(*i++, true);
            }
        }
    }

    void AsyncJob::Error(ValueException& e)
    {
        this->hadError = true;
        this->OnError(e);

        std::vector<TiMethodRef>::iterator i = this->errorCallbacks.begin();
        while (i != this->errorCallbacks.end())
        {
            this->DoCallback(*i++, false);
        }
    }

    void AsyncJob::AddProgressCallback(TiMethodRef callback)
    {
        this->progressCallbacks.push_back(callback);
    }

    void AsyncJob::AddCompletedCallback(TiMethodRef callback)
    {
        this->completedCallbacks.push_back(callback);
    }

    void AsyncJob::AddErrorCallback(TiMethodRef callback)
    {
        this->errorCallbacks.push_back(callback);
    }

    void AsyncJob::_Cancel(const ValueList& args, ValueRef result)
    {
        this->Cancel();
    }

    void AsyncJob::_GetProgress(const ValueList& args, ValueRef result)
    {
        result->SetDouble(this->GetProgress());
    }

    void AsyncJob::_IsComplete(const ValueList& args, ValueRef result)
    {
        result->SetBool(this->completed);
    }

}
