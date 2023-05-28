#ifndef SW_LIGHT_VARIANT_HPP_
#define SW_LIGHT_VARIANT_HPP_

#include <string>

namespace sowa {

enum class LightVariantType {
	None = 0,
	Int,
	Float,
	Double,
	Bool,
	String
};

struct light_variant {
  public:
	light_variant();
	light_variant(int);
	light_variant(float);
	light_variant(double);
	light_variant(bool);
	light_variant(const std::string &);
	light_variant(const char *);

	~light_variant();

	light_variant(const light_variant&);

	int Int() const;
	float Float() const;
	double Double() const;
	bool Bool() const;
	std::string String() const;

	void SetValue(int);
	void SetValue(float);
	void SetValue(double);
	void SetValue(bool);
	void SetValue(const std::string &);
	void SetValue(const char *);
	void RemoveValue();

	std::string Repr();

	inline const LightVariantType Type() const { return variantType; }

  private:
	union {
		int int_;
		float float_;
		double double_;
		bool bool_;

		std::string *string_;
	} value;

	LightVariantType variantType = LightVariantType::None;
};
} // namespace sowa

#endif // SW_LIGHT_VARIANT_HPP_