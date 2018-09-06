/*-------------------------------------------------------------------------
FileName     : fincall.h
Description  : 跟出块作用域时自动函数/方法调用
remark       : 利用析构的特性
Modification :
--------------------------------------------------------------------------
   1、Date  2018-01-23       create     hejl 
-------------------------------------------------------------------------*/

// 通过局部对象析构自动释放锁
template<class T>
class FinCall
{
	typedef void (T::*TFun)(void);
public:
	FinCall(T& obj, TFun out_func):
	  m_obj(obj), m_outf(out_func) {/* if (in_func)(obj.*in_func)();*/}   

	  ~FinCall() { (m_obj.*m_outf)(); }

private:
	T& m_obj;
	TFun m_outf;
};
