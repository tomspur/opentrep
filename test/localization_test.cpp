#include "tests_common.hpp"


#include "../examples/locale_less.hpp"


int localization_test()
{
	long result = errors_counter();

	typedef containers::ternary_tree<std::string, int, utility::locale_less<char> > LocTst;
	LocTst names(utility::locale_less<char>::locale_less(utility::swedish_locale_name));

	names["a�a"] = 2;
	names["a��"] = 3;
	names["aa�"] = 1;
	names["a��"] = 4;
	names["���"] = 11;
	names["���"] = 12;
	names["���"] = 17;
	names["���"] = 14;
	names["���"] = 16;
	names["���"] = 15;
	names["���"] = 13;

	bool display = false;
	BOOST_CHECK(names.size() == 11);
	LocTst::iterator it = names.begin();
	int x = 0, prev = 0;
	while (it != names.end())
	{
		x = *it++;
		BOOST_CHECK(x > prev);
		if (x <= prev) {
			display = true;
			break;
		}
		prev = x;
	}
	if (display) {
		for (it = names.begin(); it != names.end(); ++it) {
			std::string key = it.key();
			CharToOem(key.c_str(), (char*)key.c_str());
			std::cout << key << ", ";
		}
	}

	return errors_counter() - result;
}

