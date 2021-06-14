#include "stmdsp.hpp"
//#include "exprtk.hpp"
#include <string>
#include <vector>

std::vector<stmdsp::dacsample_t> siggen_formula_parse(const std::string& formulaString)
{
    double x = 0;

    //exprtk::symbol_table<double> symbol_table;
    //symbol_table.add_variable("x", x);
    //symbol_table.add_constants();

    //exprtk::expression<double> expression;
    //expression.register_symbol_table(symbol_table);

    //exprtk::parser<double> parser;
    //parser.compile(formulaString, expression);

    std::vector<stmdsp::dacsample_t> samples;
    for (x = 0; samples.size() < stmdsp::SAMPLES_MAX; x += 1) {
        //auto y = static_cast<stmdsp::dacsample_t>(expression.value());
        samples.push_back(2048);
    }

    return samples;
}

