#pragma once
extern std::string empty;

/// Command Variable

// useful macros
#define ComVarFromPtr(_var_name, _var_type, _var_ptr, _command_name, _description, _min_parameter_count, _max_parameter_count, _callback) \
const ComVar<_var_type> _var_name##__LINE__(_var_ptr); \
ConsoleCommand _var_name((ComVarBase*)&_var_name##__LINE__, _command_name, _description, _min_parameter_count, _max_parameter_count, _callback)

#define ComVarFromPtrIpv4(_var_name, _var_ptr, _command_name, _description, _min_parameter_count, _max_parameter_count, _callback) \
const ComVarAddrIpv4 _var_name##__LINE__(_var_ptr); \
ConsoleCommand _var_name((ComVarBase*)&_var_name##__LINE__, _command_name, _description, _min_parameter_count, _max_parameter_count, _callback)

class ComVarBase
{
public:
	ComVarBase() = default;
	virtual ~ComVarBase() = default;

	virtual std::string AsString() const
	{
		return "<unimplemented>";
	}
};

template<typename T>
class CStrToValue
{
public:
	template<typename Type = T>
	static Type ToIntegral(const std::string& str, int _Base);

	template<>
	static int ToIntegral<int>(const std::string& str, int _Base)
	{
		return std::stoi(str, nullptr, _Base);
	}

	// cool, signed char is apparently considered different from char
	// at least for MSVC, requiring implementing both specializations
	template<>
	static signed char ToIntegral<signed char>(const std::string& str, int _Base)
	{
		return ToIntegral<int>(str, _Base);
	}

	// cool, signed char is apparently considered different from char
	// at least for MSVC, requiring implementing both specializations
	template<>
	static signed short ToIntegral<signed short>(const std::string& str, int _Base)
	{
		return ToIntegral<int>(str, _Base);
	}

	template<>
	static unsigned int ToIntegral<unsigned int>(const std::string& str, int _Base)
	{
		return std::stoul(str, nullptr, _Base);
	}

	template<>
	static unsigned short ToIntegral<unsigned short>(const std::string& str, int _Base)
	{
		return ToIntegral<unsigned int>(str, _Base);
	}

	template<>
	static long ToIntegral<long>(const std::string& str, int _Base)
	{
		return std::stol(str, nullptr, _Base);
	}

	template<>
	static unsigned long ToIntegral<unsigned long>(const std::string& str, int _Base)
	{
		return std::stoul(str, nullptr, _Base);
	}

	template<>
	static long long ToIntegral<long long>(const std::string& str, int _Base)
	{
		return std::stoll(str, nullptr, _Base);
	}

	template<typename Type = T>
	static bool ToBoolean(const std::string& str)
	{
		bool result = false;
		if (str != "true"
			&& str != "false"
			&& str != "1"
			&& str != "0"
			)
			throw std::runtime_error("error: invalid boolean format (must be true/false)");
		else
			result = str == "true" ? true : false;

		return result;
	}

	template<typename Type = T>
	static Type ToFloat(const std::string& str);

	template<>
	static float ToFloat<float>(const std::string& str)
	{
		return std::stof(str);
	}

	template<>
	static double ToFloat<double>(const std::string& str)
	{
		return std::stod(str);
	}
};

template<typename T>
class ComVar : protected CStrToValue<T>, public ComVarBase
{
protected:
	T* m_var_ptr;

	template<typename Type = T>
	std::string ToString() const
	{
		return std::to_string(*m_var_ptr);
	}

	template<>
	std::string ToString<bool>() const
	{
		return *m_var_ptr ? "true" : "false";
	}

	template<>
	std::string ToString<unsigned short>() const
	{
		return ToString<unsigned int>();
	}

public:
	// for custom types, the class should implement the equal operator overload
	// othewise default is used
	ComVar(T* ptr)
	{
		m_var_ptr = ptr;
	}

	virtual ~ComVar() = default;

	template<typename Type = T>
	std::enable_if_t<!std::is_same_v<Type, bool> && std::is_integral_v<Type>, bool> 
		SetFromStr(const std::string& str, int _Base = 0, std::string& potentialException = empty)
	{
		bool success = true;
		try
		{
			*m_var_ptr = this->ToIntegral<Type>(str, _Base);
		}
		catch (const std::exception& e)
		{
			success = false;
			potentialException.assign(e.what());
		}

		return success;
	}

	template<typename Type = T>
	std::enable_if_t<std::is_same_v<Type, bool>, bool>
		SetFromStr(const std::string& str, std::string& potentialException = empty)
	{
		bool success = true;
		try
		{
			*m_var_ptr = this->ToBoolean<Type>(str);
		}
		catch (const std::exception& e)
		{
			success = false;
			potentialException.assign(e.what());
		}

		return success;
	}

	template<typename Type = T>
	std::enable_if_t<std::is_floating_point_v<Type>, bool>
		SetFromStr(const std::string& str, std::string& potentialException = empty)
	{
		bool success = true;
		try
		{
			*m_var_ptr = this->ToFloat<Type>(str);
		}
		catch (const std::exception& e)
		{
			success = false;
			potentialException.assign(e.what());
		}

		return success;
	}

	std::string AsString() const override
	{
		return ToString();
	}

	T GetVal() const
	{
		return *m_var_ptr;
	}

	void SetVal(T val)
	{
		*m_var_ptr = val;
	}
};

class ComVarAddrIpv4 : public ComVar<unsigned long>
{
public:
	ComVarAddrIpv4(unsigned long* ptr)
		: ComVar<unsigned long>(ptr)
	{
	}

	std::string AsString() const override
	{
		return std::to_string(*m_var_ptr & 0x000000FF)			+ '.'
			+ std::to_string((*m_var_ptr & 0x0000FF00) >>  8)	+ '.'
			+ std::to_string((*m_var_ptr & 0x00FF0000) >> 16)	+ '.'
			+ std::to_string((*m_var_ptr & 0xFF000000) >> 24);
	}

	bool SetFromStr(const std::string& str, std::string& potentialException = empty)
	{
		bool success = true;
		try
		{
			//std::string subip_address("");
			//size_t cur_offset = 0;

			unsigned long new_address = 0;

			if (str.length() < std::string("0.0.0.0").length()
				|| str.length() > std::string("255.255.255.255").length()
				|| str.find_first_not_of("0123456789.") != std::string::npos)
			{
				throw std::runtime_error("invalid ipv4 address format");
			}

			/*
			for (int i = 0; i < 3; i++)
			{
				size_t dot_occurrance = str.find_first_of('.', cur_offset);
				if (dot_occurrance == std::string::npos
					|| dot_occurrance <= cur_offset
					|| dot_occurrance - cur_offset < 1)
				{
					throw std::runtime_error("invalid ipv4 address format");
				}

				subip_address = str.substr(cur_offset, dot_occurrance - cur_offset);

				if (ComVar::ToIntegral<unsigned long>(subip_address, 10) != ComVar::ToIntegral<unsigned long>(subip_address, 0))
				{
					throw std::runtime_error("invalid ipv4 address format");
				}

				new_address |= (ComVar::ToIntegral<unsigned long>(subip_address, 10) & 0xFF) << (i * 8);
				cur_offset = dot_occurrance + 1;
			}

			subip_address = str.substr(cur_offset, str.length() - cur_offset);

			if (ComVar::ToIntegral<unsigned long>(subip_address, 10) != ComVar::ToIntegral<unsigned long>(subip_address, 0))
			{
				throw std::runtime_error("invalid ipv4 address format");
			}

			new_address |= ComVar::ToIntegral<unsigned long>(subip_address, 10) & 0xFF << 24;
			*/

			//*m_var_ptr = inet_addr(str.c_str());
		}
		catch (const std::exception& e)
		{
			success = false;
			potentialException.assign(e.what());
		}

		return success;
	}
};