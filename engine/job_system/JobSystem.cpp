#include "JobSystem.hpp"

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<MFA::JobSystem> MFA::JobSystem::Instance()
{
    std::shared_ptr<MFA::JobSystem> shared_ptr = _instance.lock();
	if (shared_ptr == nullptr)
	{
		shared_ptr = std::make_shared<MFA::JobSystem>();
		_instance = shared_ptr;
	}
	return shared_ptr;
}

//----------------------------------------------------------------------------------------------------------------------

MFA::JobSystem::JobSystem() = default;
MFA::JobSystem::~JobSystem() = default;

//----------------------------------------------------------------------------------------------------------------------