#pragma once

#include <atomic>
#include <condition_variable>

// TODO: this seems weird. Why not have a seperate header for ITokenTracker?
#include "Token.h"                // Inherits from ITokenTracker.

namespace BitFunnel
{
    //*************************************************************************
    //
    // TokenTracker implements ITokenTracker and provides a way to track 
    // tokens issued before a particular cutoff serial number.
    // TokenTracker gets the cutoff serial number and the number of tokens
    // currently in-flight at construction. Whenever it receives a callback 
    // from a token with serial number less than the cutoff value, it 
    // decrements the remaining token count. When remaining token count reaches
    // zero, tracker has finished tracking the tokens of interest.
    // Consumers will consult this class whether the tracking is complete via
    // IsComplete(non blocking) or WaitForCompletiong(blocking).
    //
    // Potentially there can be multiple trackers which track an overlapping 
    // set of tokens. Even though the tokens can be returned in a different 
    // order than they were created, it still means that the tracker which was
    // started earlier will never finish after the one which was started later.
    //
    //*************************************************************************
    class TokenTracker : public ITokenTracker,
                         private NonCopyable
    {
    public:

        // Constructs a tracker to track tokens issued before a cut off serial 
        // number
        TokenTracker(SerialNumber cutoffSerialNumber, unsigned remainingTokenCount);

        ~TokenTracker();

        // Token manager calls OnTokenComplete each time a token goes out of
        // scope. This method returns true if it has collected enough 
        // notifications and is no longer interested in being notified. This 
        // means the tracker has fulfilled its job and all of the tokens in 
        // flight need no tracking from it.
        // This method is thread safe.
        bool OnTokenComplete(SerialNumber serialNumber);

        //
        // ITokenTracker API
        //
        virtual bool IsComplete() const override;
        virtual void WaitForCompletion() override;

    private:

        // Cutoff serial number of the tokens of interest. This is 
        // a non-inclusive range.
        const SerialNumber m_cutoffSerialNumber;

        // Number of tokens of interest which are still in flight.
        std::atomic<unsigned int> m_remainingTokenCount;

        std::condition_variable m_condition;
        std::mutex m_conditionLock;
    };
}
