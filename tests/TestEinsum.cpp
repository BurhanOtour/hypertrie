#include <catch2/catch.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <set>
#include <unordered_set>
#include <set>
#include <iostream>

#include <Dice/hypertrie/boolhypertrie.hpp>

#include <torch/torch.h>

#include "utils/GenerateTriples.hpp"
#include "einsum/EinsumTestData.hpp"


namespace hypertrie::tests::einsum {

template <typename T>
void runTest(long excl_max, TestEinsum &test_einsum) {
    auto einsum = &bh_ns::einsum2map<T>;
    // expected result
    torch::Tensor expected_result = torch::einsum(test_einsum.str_subscript, test_einsum.torchOperands());
//		WARN(expected_result);
    // result how it is
    auto actual_result = einsum(test_einsum.subscript, test_einsum.hypertrieOperands());

    unsigned long result_depth = test_einsum.subscript->resultLabelCount();
    for (const auto &key : product<std::size_t>(result_depth, excl_max)) {
        auto actual_entry = (actual_result.count(key)) ? actual_result[key] : 0;
        auto expected_entry = T(resolve(expected_result, key));  // to bool
//			WARN("key: ({})"_format(fmt::join(key, ", ")));
//			WARN("expected: {}, actual {}"_format(resolve(expected_result, key), actual_entry));
        REQUIRE (actual_entry == expected_entry);
    }
}

template <typename T>
void runTest(long excl_max, std::vector<TestOperand> &operands, const std::shared_ptr<Subscript> &subscript) {
    TestEinsum test_einsum{subscript, operands};
    runTest<T>(excl_max, test_einsum);
}

template <typename T>
void runSubscript(std::string subscript_string, long excl_max = 4, bool empty = false, std::size_t runs = 15) {
	static std::string result_type_str = std::is_same_v<T, bool> ? "bool" : "ulong";
    SECTION("{} [res:{}]"_format(subscript_string, result_type_str)) {
        for (std::size_t run : iter::range(runs))
            SECTION("run {}"_format(run)) {
                torch::manual_seed(std::hash<std::size_t>()(run));
                auto subscript = std::make_shared<Subscript>(subscript_string);
                std::vector<TestOperand> operands{};
                for (const auto &operand_sc : subscript->getRawSubscript().operands) {
                    TestOperand &operand = operands.emplace_back(operand_sc.size(), excl_max, empty);
//						WARN(operand.torch_tensor);
                }
                runTest<T>(excl_max, operands, subscript);

            }
    }
}

TEST_CASE("problematic test cases", "[einsum]") {

    std::vector<std::string> subscript_strs{
            "abc,dcebf,gdghg,bdg,ijibg->c", // is calculated faster
            "abc,dcebf,gdghg,ijibg->c", // its minimal
            "abcd,ceffb,cfgaf,hbgi,ccfaj->j",
            "abbc,d,ebcfg,hdif,hhchj->b"

    };
    for (bool empty : {false, true})
        SECTION("empty = {}"_format(empty)) {
            for (auto excl_max : {10}) {
                SECTION("excl_max = {}"_format(excl_max)) {
                    for (auto subscript_str : subscript_strs) {
                        runSubscript<std::size_t>(subscript_str, excl_max, empty, 1);
						runSubscript<bool>(subscript_str, excl_max, empty, 1);
                    }
                }
            }
        }

}


TEST_CASE("run simple cases", "[einsum]") {

    std::vector<std::string> subscript_strs{
            "a->a",
            "ab->a",
            "ab->b",
            "ab->ab",
            "ab->ba",
            "a,a->a",
            "ab,a->a",
            "ab,a->b",
            "ab,a->ab",
            "ab,a->ba",
            "a,b->a",
            "a,b->b",
            "a,b->ab",
            "aa,bb->ab",
            "aa,bb->b",
            "aa,bb->a",
            "ac,cb->c",
            "ac,cb->b",
            "a,b,c->abc",
            "a,b,c->ac",
            "a,b,c->ca",
            "a,b,c->a",
            "a,b,c->c",
            "a,b,cd->d",
            "a,bbc,cdc,cf->f",
            "aa,ae,ac,ad,a,ab->abcde"

    };
    for (bool empty : {false, true}){
        SECTION("empty = {}"_format(empty))
            for (auto excl_max : {4, 7, 10, 15}) {
                SECTION("excl_max = {}"_format(excl_max))
                    for (auto subscript_str : subscript_strs) {
                        runSubscript<std::size_t >(subscript_str, excl_max, empty);
                        runSubscript<bool>(subscript_str, excl_max, empty);
                    }
                }

        }

}

template <typename T>
void generate_and_run_exec(long excl_max, TestEinsum &test_einsum) {
    SECTION("{}"_format(test_einsum.str_subscript)) {
        runTest<T>(excl_max, test_einsum);
    }
}



struct GenerateAndRunSetup {
    long excl_max;
    std::vector<TestOperand> test_operands;
    std::vector<TestEinsum> test_einsums;
};


TEST_CASE("generate and run", "[einsum]") {
    static GenerateAndRunSetup setup = []() -> GenerateAndRunSetup {
        torch::manual_seed(std::hash<std::size_t>()(42));
        long excl_max = 10;
        std::size_t test_operands_count = 5;
        std::vector<TestEinsum> test_einsums;

        std::vector<TestOperand> test_operands;
        test_operands.reserve(test_operands_count);
        // generate tensors
        for (auto r: gen_random<uint8_t>(test_operands_count, 1, 5)) {
            TestOperand &operand = test_operands.emplace_back(r, excl_max);
            UNSCOPED_INFO("Operand generated: depth = {}, dim_range = [0,{}), nnz = {}"_format(operand.depth,
                                                                                               operand.excl_max,
                                                                                               operand.hypertrie.size()));
        }
        std::size_t test_einsums_count = 3;
        test_einsums.reserve(test_einsums_count);

        for ([[maybe_unused]]auto i : iter::range(test_einsums_count))
            test_einsums.emplace_back(test_operands);

        return {excl_max, std::move(test_operands), std::move(test_einsums)};
    }();

    for (auto &test_einsum :setup.test_einsums) {
        SECTION("{}[res:ulong]"_format(test_einsum.str_subscript)) {
            runTest<std::size_t>(setup.excl_max, test_einsum);
        }
        SECTION("{}[res:bool]"_format(test_einsum.str_subscript)) {
            runTest<bool>(setup.excl_max, test_einsum);
        }
    }


}

TEST_CASE("nothing", "[einsum]") {
    // do nothing;
}

}
