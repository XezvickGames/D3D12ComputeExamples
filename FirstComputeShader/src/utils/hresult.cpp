#include "pch.hpp"
#include "hresult.hpp"

HrException::HrException(const std::string& msg) : runtime_error(msg.c_str()) {}
HrException::HrException(const char* msg) : runtime_error(msg) {}


HrException logHresult(HRESULT hr, const int line, const char* file, const char* func, const char* call)
{
	std::string text("\n----------------------------------------------------------------------------------------\n");
	text.append(std::format(
		"ERROR: {}\n call: \"{};\"\n{}({}:{})\nERROR CODE(HRESULT) : {}",
		translateHresult(hr), call, file, func, line, hr));
	text.append(("\n----------------------------------------------------------------------------------------\n"));

	std::cerr << text << std::endl;
	OutputDebugStringA(text.c_str());
	MessageBoxA(nullptr, text.c_str(), "ERROR", MB_ICONERROR);
	return HrException(text);
}

std::string translateHresult(HRESULT hresult)
{
	char* msg_handle = nullptr;
	auto length = FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, hresult,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<LPSTR>(&msg_handle), 0, nullptr);
	std::string temp("");
	if (!length)
	{
		temp.append("Cant retrieve description of current error, possible FormatMessage status:\n");

		std::string sub_error = translateHresult(HRESULT_FROM_WIN32(GetLastError())).c_str();
		temp.append(" \" ").append(sub_error).append(" \" ");
	}
	else
	{
		temp.append(msg_handle);
	}

	if (temp.back() == '\n') { temp.pop_back(); }
	if (temp.back() == '\r') { temp.pop_back(); }

	if (LocalFree(msg_handle))
	{
		std::stringstream ss;
		ss << "\n\n!!!LocalFree of 0x" << std::hex << (long long)msg_handle << std::dec << " failed \n";
		temp.append(ss.str());
		ss.clear();

		std::string sub_error = translateHresult(HRESULT_FROM_WIN32(GetLastError())).c_str();
		temp.append(" \" ").append(sub_error).append(" \" ");
	}

	return temp;
}