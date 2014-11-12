//Copyright (C) 2014, guardiancrow

#pragma once

#include <string>
#include <sstream>

///文字列に置き換えます
///例えば実数を文字列に変換します
template<class T> inline std::string toString(T x)
{
	std::ostringstream sout;
	sout<<x;
	return sout.str();
}

