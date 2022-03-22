#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> unique_lock(_mutex);
    _cond.wait(unique_lock, [this] { return !_messages.empty();});
    T msg = std::move(_messages.front());
    _messages.pop_front();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock_guard(_mutex);
    _messages.push_back(std::move(msg));
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::RED;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (_messageQueue.receive() == TrafficLightPhase::GREEN)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    std::mt19937_64 eng{std::random_device{}()};
    std::uniform_int_distribution<> dist{4000, 6000};
    auto duration = dist(eng);

    auto lastTime = std::chrono::system_clock::now();

    while (true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      auto curTime = std::chrono::system_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastTime).count() >= duration)
      {
          switch (_currentPhase)
          {
            case TrafficLightPhase::RED:
                _currentPhase = TrafficLightPhase::GREEN;
                break;
            case TrafficLightPhase::GREEN:
                _currentPhase = TrafficLightPhase::RED;
                break;
          }
          
          TrafficLightPhase currentPhase = _currentPhase;
          auto sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_messageQueue, std::move(currentPhase));
          sent.wait();


          duration = dist(eng);
          lastTime = std::chrono::system_clock::now();
          
      }


  }
}
