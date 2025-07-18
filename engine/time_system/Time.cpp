#include "Time.hpp"

#include "../bedrock/BedrockAssert.hpp"

#include <SDL2/SDL_timer.h>

namespace MFA
{
    
    //==========================================================

    std::unique_ptr<Time> Time::Instantiate(int maxFramerate, int minFramerate)
    {
        return std::make_unique<Time>(maxFramerate, minFramerate);
    }

    //==========================================================

    Time::Time(int maxFramerate,int minFramerate)
    {
        MFA_ASSERT(Instance == nullptr);
        Instance = this;
        _startTimeMs = SDL_GetTicks();
        _nowMs = _startTimeMs;
        _minDeltaTimeMs = static_cast<int>(1000.0f / static_cast<float>(maxFramerate));
        _deltaTimeSec = static_cast<float>(_minDeltaTimeMs) / 1000.0f;
        _maxDeltaTimeMs = static_cast<int>(1000.0f / static_cast<float>(minFramerate));
    }
    
    //==========================================================
    
    Time::~Time()
    {
        MFA_ASSERT(Instance != nullptr);
        Instance = nullptr;
    }

    //==========================================================
    
    void Time::Update()
    {
        _deltaTimeMs = SDL_GetTicks() - _nowMs;
        if (_minDeltaTimeMs > _deltaTimeMs)
        {
            SDL_Delay(_minDeltaTimeMs - _deltaTimeMs);
        }

        _deltaTimeMs = SDL_GetTicks() - _nowMs;
        _deltaTimeMs = std::min(_deltaTimeMs, _maxDeltaTimeMs);
        _deltaTimeSec = static_cast<float>(_deltaTimeMs) / 1000.0f;
        _timeSec += _deltaTimeSec;

        _nowMs = SDL_GetTicks();

        {
            int const count = (int)_pUpdateTasks.size();
            for (int i = 0; i < count; i++)
            {
                auto task = _pUpdateTasks.front();
                _pUpdateTasks.pop();
                if (task() == true)
                {
                    _pUpdateTasks.push(task);
                }
            }
        }
        {
            int const count = (int)_updateTasks.ItemCount();
            for (int i = 0; i < count; i++)
            {
                UpdateTask task{};
                bool isEmpty{};
                if (_updateTasks.TryToPop(task, isEmpty))
                {
                    if (task() == true)
                    {
                        _pUpdateTasks.emplace(task);
                    }
                }
            }
        }
    }

    //==========================================================

    void Time::AddUpdateTask(UpdateTask task)
    {
        if (Instance == nullptr)
        {
            return;
        }
        Instance->_updateTasks.Push(std::move(task));
    }

    //==========================================================

    int Time::DeltaTimeMs()
    {
        return Instance->_deltaTimeSec;
    }

    //==========================================================

    float Time::DeltaTimeSec()
    {
        return Instance->_deltaTimeSec;
    }

    //==========================================================
    
    float Time::NowSec() { return Instance->_timeSec; }

    //==========================================================

    bool Time::HasInstance()
    {
        return Instance != nullptr;
    }

    //==========================================================
    
}