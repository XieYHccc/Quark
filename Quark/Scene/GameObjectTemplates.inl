#pragma once

namespace quark {

template<typename T, typename... Args>
T& GameObject::AddComponent(Args&&... args)
{
	return m_Scene->m_Registry.Register<T>(m_Entity, std::forward<Args>(args)...);
}

template<typename T>
T& GameObject::GetComponent()
{
	return m_Entity->GetComponent<T>();
}

template<typename T>
const T& GameObject::GetComponent() const
{
	return m_Entity->GetComponent<T>();
}

template<typename T>
bool GameObject::HasComponent()
{
	return m_Entity->HasComponent<T>();
}


}
