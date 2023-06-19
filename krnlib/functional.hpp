#pragma once
#include <krnlib/memory.hpp>
#include <type_traits>

namespace krnlib
{
namespace detail
{
/* ---------------------------------------------------------------------------------------------
* 1: ���ɵ��ö����װΪһ����, �������
* 2: ʹ�����Ͳ���, ʹ�����������
--------------------------------------------------------------------------------------------- */
template <class RetT, class... ArgsT>
class __declspec(novtable) CallableObjBase
{
public:
	/*
	* ע: ����˵��"����", ��������(ʵ����)����ת�ͺ��
	* ʹ�����Ͳ�����ϸ����������, ʵ�ʵ���ʱִ�е��������Ӧ�ĺ���
	*/

	// ����һ���µĶ���, ��ȡ�����ĸ���
	virtual CallableObjBase* GetCopy() const = 0;
	// ���ÿɵ��ö���
	virtual RetT DoCall(ArgsT&&...) = 0;

	CallableObjBase() = default;
	CallableObjBase(const CallableObjBase&) = delete;
	CallableObjBase& operator=(const CallableObjBase&) = delete;
};

/* ---------------------------------------------------------------------------------------------
* �����FuncBase��ʵ����, ���Ͳ��������صĲ���
--------------------------------------------------------------------------------------------- */
template <class CallableT, class RetT, class... ArgsT>
class CallableObjImpl : public CallableObjBase<RetT, ArgsT...>
{
public:
	using InheritedT = CallableObjBase<RetT, ArgsT...>;

	/*
	* �Զ�����FuncTTemp������ִ�и�ֵ������ֵ����
	* ʹ��enable_if_tȷ��FuncTTemp��������������, ������������޵ݹ�
	*/
	template<class NewCallableT, std::enable_if_t<!std::is_same_v<CallableObjImpl, std::decay_t<NewCallableT>>, int> = 0>
	CallableObjImpl(NewCallableT&& new_call) : call_(std::forward<NewCallableT>(new_call)) {}

private:
	InheritedT* GetCopy() const override {
		/*
		* �ȷ���һ��������С���ڴ�
		* Ȼ����ڴ�ִ�й��캯��, �����ڲ��Ŀɵ��ö���
		* ��󷵻ظ���������Ͳ���
		*/
		return New<CallableObjImpl>(call_);
	}

	RetT DoCall(ArgsT&&... args) override {
		return std::invoke(call_, std::forward<ArgsT>(args)...);
	}

	CallableT call_;
};


/* ---------------------------------------------------------------------------------------------
* �����������
* 1. ���ɵ��ö���Ĳ����ٷ�װһ��
* 2. ʹ����������ֻ��Ҫ�ṩһ��ģ��, �Զ���ģ���еķ���ֵ���ͺͲ����б���ȡ����
--------------------------------------------------------------------------------------------- */
template <class RetT, class... ArgsT>
class FuncBaseImpl
{
public:
	using CallableObj = CallableObjBase<RetT, ArgsT...>;

	FuncBaseImpl() : callable_obj_ptr_(nullptr) {}
	~FuncBaseImpl() { Tidy(); }

	// �����ڲ��Ŀɵ��ö���
	RetT operator()(ArgsT... args) const {
		return callable_obj_ptr_->DoCall(std::forward<ArgsT>(args)...);
	}

protected:
	// �ж��Ƿ��ǿɵ��ö���, ���Ҵ����NewCallableT�������ĵ��÷�ʽ��ͬ
	template <class NewCallableT, class function>
	using EnableIfAllowableCallT = std::enable_if_t<std::conjunction_v<std::negation<std::is_same<std::_Remove_cvref_t<NewCallableT>, function>>,
		std::_Is_invocable_r<RetT, std::decay_t<NewCallableT>, ArgsT...>>,
		int>;
	/*
	* EnableIfAllowableCallTԭ��:
	* 1: std::negation<std::is_same<std::_Remove_cvref_t<CallableT>, function>>
	*	����: ����ͨ��std::_Remove_cvref_t<CallableT>ȥ��CallableT��һЩ��������Ȼ��ʹ��std::is_same��function�ж�, ���ʹ��std::negation�����ȡ��
	*	����: ��ֹCallableT��������function���, ���¿������캯��ʱ�������޵ݹ�
	*
	* 2: std::_Is_invocable_r<RetT, std::decay_t<CallableT>&, ArgsT...>>
	*	����: ����ͨ��std::decay_t<CallableT>ȥ��CallableT���ŵ�����, Ȼ��ʹ��std::_Is_invocable_r���ɵ��ö����Ƿ���԰��մ���ķ���ֵ�Ͳ����б����е���
	*	����: ��������CallableT�Ƿ���԰���Ԥ�ڵ���
	*
	* 3: ���ͨ��std::conjunction_v�� 1 & 2
	*/


	// ����Ϊ�����ĸ���
	void ResetCopy(const FuncBaseImpl& right) {
		if (!right.Empty())
			callable_obj_ptr_ = right.callable_obj_ptr_->GetCopy();
	}

	// ����Ϊ����������
	void ResetMove(FuncBaseImpl&& right) noexcept {
		if (!right.Empty()) {
			callable_obj_ptr_ = right.callable_obj_ptr_;
			right.callable_obj_ptr_ = nullptr;
		}
	}

	// �����ڲ��Ŀɵ��ö���
	template <class CallableT>
	void AutoReset(CallableT&& call)
	{
		// ��ȡCallableObjBase��ʵ����
		using ImplT = CallableObjImpl<std::decay_t<CallableT>, RetT, ArgsT...>;
		// �����ڴ�, ����������ת��(���Ͳ���)
		callable_obj_ptr_ = (CallableObj*)New<ImplT>(std::forward<CallableT>(call));
	}

	bool Empty() const noexcept {
		return callable_obj_ptr_ == nullptr;
	}

	// ����
	void Swap(FuncBaseImpl& right) noexcept {
		CallableObj* temp = callable_obj_ptr_;
		callable_obj_ptr_ = right.callable_obj_ptr_;
		right.callable_obj_ptr_ = temp;
	}

private:
	// �ͷ��ڴ�
	void Tidy() {
		if (callable_obj_ptr_) {
			Delete(callable_obj_ptr_);
			callable_obj_ptr_ = nullptr;
		}
	}

	CallableObj* callable_obj_ptr_;
};

/*
* ʹ��SFINAE, ����T������ѡ���Ӧ��ʵ��·��
* ����޷�ת����RetT(ArgsT...), ��ᴥ�������ڴ���
*/
template <class T>
struct GetFunctionImpl {
	static_assert(std::_Always_false<T>, "function only accepts function types as template arguments.");
};

template <class RetT, class... ArgsT>
struct GetFunctionImpl<RetT(ArgsT...)> {
	using Type = FuncBaseImpl<RetT, ArgsT...>;
};
}


template<class CallableT>
class function : public detail::GetFunctionImpl<CallableT>::Type
{
public:
	using InheritedT = typename detail::GetFunctionImpl<CallableT>::Type;

	function() noexcept {}
	function(nullptr_t) noexcept {}
	function(const function& right) {
		this->ResetCopy(right);
	}
	function(function&& right) noexcept {
		this->ResetMove(std::move(right));
	}

	/*
	* ע: �����ʹ��NewCallableT, ������ʹ�����е�CallableT, ������Ϊ������CallableT���ٶ����ʵ�������������.
	* ��Ȼ�����NewCallableT�Ƿ��ܹ���ͬ��CallableT������, �������ڲ���EnableIfAllowableCallT�����ж�
	*/
	// Ϊ�ɵ��ö�������ڴ�
	template <class NewCallableT, typename InheritedT::template EnableIfAllowableCallT<NewCallableT, function> = 0>
	function(NewCallableT&& new_call) {
		this->AutoReset(std::forward<NewCallableT>(new_call));
	}

	function& operator=(const function& right) {
		/*
		* ���ÿ������캯��, ����һ����ʱ����, ����������ݺ���������
		* �������ĺô���:
		* 1. ���Ե������еĿ������캯��, ��������, �����ظ�����
		* 2. ʹ��Swap��ԭ�е����ݽ����뿽�����캯����, �������뿪��ͻ��Զ�������������, �ͷ�ԭ�е�����
		*/
		function(right).Swap(*this);
		return *this;
	}

	function& operator=(function&& right) noexcept {
		if (this != std::addressof(right)) {
			function(std::move(right)).Swap(*this);
		}
		return *this;
	}

	// Ϊ�ɵ��ö���ֵ����
	template <class NewCallableT, typename InheritedT::template EnableIfAllowableCallT<NewCallableT, function> = 0>
	function& operator=(NewCallableT&& new_call) {
		function(std::forward<NewCallableT>(new_call)).Swap(*this);
		return *this;
	}
	~function() {}
};
}