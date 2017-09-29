
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "testing.h"

#include <tropter/tropter.h>

#include <unsupported/Eigen/Splines>

using Eigen::Ref;
using Eigen::VectorXd;
using Eigen::RowVectorXd;
using Eigen::Vector2d;
using Eigen::MatrixXd;
using Eigen::Matrix2d;

using namespace tropter;
using namespace Catch;

// This test ensures that user-specified initial guesses for an optimal
// control problem are used correctly.

/// This problem seeks to move a point mass to a specified final position with
/// minimum effort. There are two final positions (+/- 1/sqrt(2)) that are
/// equally desirable; the initial guess should determine which final position
/// the optimizer finds.
template<typename T>
class FinalPositionLocalOptima : public tropter::OptimalControlProblem<T> {
public:
    FinalPositionLocalOptima()
    {
        this->set_time(0, 1);
        this->add_state("x", {-1.5, 1.5}, {0});
        this->add_state("v", {-10, 10}, {0}, {0});
        this->add_control("F", {-50, 50});
    }
    void calc_differential_algebraic_equations(
            const DAEInput<T>& in, DAEOutput<T> out) const override {
        out.dynamics[0] = in.states[1];
        out.dynamics[1] = in.controls[0];
    }
    void calc_integral_cost(const T& /*time*/,
                       const VectorX<T>& /*states*/,
                       const VectorX<T>& controls,
                       T& integrand) const override {
        integrand = 0.001 * pow(controls[0], 2);
    }
    /// This function has minima at `x = \pm 1/\sqrt(2)`.
    static T two_minima(const T& x) {
        // Root at -1, double root at 0, and root at 1.
        // These roots cause two minima, one between -1 and 0, and another
        // between 0 and 1.
        return (x - 1) * (x + 1) * x*x;
    }
    void calc_endpoint_cost(const T& /*final_time*/,
                       const VectorX<T>& final_states,
                       T& cost) const override {
        cost = 100.0 * two_minima(final_states[0]);
    }
};

TEST_CASE("Final position cost with two local optima", "[initial_guess]") {

    // Guess low.
    {
        auto ocp = std::make_shared<FinalPositionLocalOptima<adouble>>();
        const int N = 20;
        DirectCollocationSolver<adouble> dircol(ocp, "trapezoidal", "ipopt", N);

        // TODO allow getting a guess template, so that we don't need to
        // manually fill in all parts of the guess.
        OptimalControlIterate guess;
        guess.time.setLinSpaced(N, 0, 1);
        ocp->set_state_guess(guess, "x", RowVectorXd::LinSpaced(N, 0, -1));
        ocp->set_state_guess(guess, "v", RowVectorXd::Zero(N));
        ocp->set_control_guess(guess, "F", RowVectorXd::Zero(N));
        OptimalControlSolution solution = dircol.solve(guess);
        solution.write("final_position_local_optima_low_solution.csv");
        REQUIRE(Approx(solution.states.rightCols<1>()[0]).epsilon(1e-4)
                        == -1/sqrt(2));
    }
    // Guess high.
    {
        auto ocp = std::make_shared<FinalPositionLocalOptima<adouble>>();
        const int N = 20;
        DirectCollocationSolver<adouble> dircol(ocp, "trapezoidal", "ipopt", N);

        // TODO allow getting a guess template, so that we don't need to
        // manually fill in all parts of the guess.
        OptimalControlIterate guess;
        guess.time.setLinSpaced(N, 0, 1);
        ocp->set_state_guess(guess, "x", RowVectorXd::LinSpaced(N, 0, +1));
        ocp->set_state_guess(guess, "v", RowVectorXd::Zero(N));
        ocp->set_control_guess(guess, "F", RowVectorXd::Zero(N));
        OptimalControlSolution solution = dircol.solve(guess);
        solution.write("final_position_local_optima_high_solution.csv");
        REQUIRE(Approx(solution.states.rightCols<1>()[0]).epsilon(1e-4)
                        == +1/sqrt(2));
    }
}

TEST_CASE("Exceptions for setting optimal control guess", "[initial_guess]") {
    auto ocp = std::make_shared<FinalPositionLocalOptima<adouble>>();
    int N = 15;
    DirectCollocationSolver<adouble> dircol(ocp, "trapezoidal", "ipopt", N);

    OptimalControlIterate guess;

    // Check for exceptions with OptimalControlProblem set_*_guess().
    // --------------------------------------------------------------
    // Must set guess.time first.
    REQUIRE_THROWS_WITH(ocp->set_state_guess(guess, "x", RowVectorXd::Zero(1)),
                        Contains("guess.time is empty"));
    REQUIRE_THROWS_WITH(
            ocp->set_control_guess(guess, "x", RowVectorXd::Zero(1)),
            Contains("guess.time is empty"));
    guess.time.setLinSpaced(N, 0, 1);

    // Wrong number of elements.
    REQUIRE_THROWS_WITH(ocp->set_state_guess(guess, "x", RowVectorXd::Zero(1)),
                        Contains("Expected value to have 15"));
    REQUIRE_THROWS_WITH(
            ocp->set_control_guess(guess, "F", RowVectorXd::Zero(1)),
            Contains("Expected value to have 15"));

    // Wrong state name.
    REQUIRE_THROWS_WITH(ocp->set_state_guess(guess, "H", RowVectorXd::Zero(N)),
                        Contains("State H does not exist"));
    REQUIRE_THROWS_WITH(
            ocp->set_control_guess(guess, "H", RowVectorXd::Zero(N)),
            Contains("Control H does not exist"));

    guess.states.resize(10, N - 1);
    guess.controls.resize(9, N - 2);
    // guess.states has the wrong size.
    REQUIRE_THROWS_WITH(ocp->set_state_guess(guess, "x", RowVectorXd::Zero(N)),
                        Contains("Expected guess.states to have "));
    REQUIRE_THROWS_WITH(
            ocp->set_control_guess(guess, "F", RowVectorXd::Zero(N)),
            Contains("Expected guess.controls to have "));

    // Test for more exceptions when calling solve().
    // ----------------------------------------------
    guess.time.resize(N - 10);   // incorrect.
    guess.states.resize(2, N);   // correct.
    guess.controls.resize(1, N); // correct.
    REQUIRE_THROWS_WITH(dircol.solve(guess),
            Contains("Expected time, states, and controls to have"
                    " the same number of columns (they have 5, "
                    "15, 15 columns, respectively)."));

    guess.time.resize(N);        // correct.
    guess.states.resize(6, N);   // incorrect.
    guess.controls.resize(1, N); // correct.
    REQUIRE_THROWS_WITH(dircol.solve(guess),
            Contains("Expected states to have 2 rows, but it has 6 rows."));

    guess.states.resize(2, N + 1); // incorrect.
    REQUIRE_THROWS_WITH(dircol.solve(guess),
            Contains("Expected time, states, and controls to have"
                    " the same number of columns (they have 15, "
                    "16, 15 columns, respectively)."));

    guess.states.resize(2, N);   // correct.
    guess.controls.resize(4, N); // incorrect.
    REQUIRE_THROWS_WITH(dircol.solve(guess),
            Contains("Expected controls to have 1 rows, but it has 4 rows."));

    guess.controls.resize(1, N - 3); // incorrect
    REQUIRE_THROWS_WITH(dircol.solve(guess),
            Contains("Expected time, states, and controls to have"
                    " the same number of columns (they have 15, "
                    "15, 12 columns, respectively)."));
}


TEST_CASE("(De)serialization of OptimalControlIterate", "[iterate_readwrite]") {
    // Create an iterate.
    OptimalControlIterate it0;
    int num_times = 15;
    int num_states = 3;
    int num_controls = 2;
    it0.time.resize(num_times);
    it0.time.setRandom();

    it0.states.resize(num_states, num_times);
    it0.states.setRandom();

    it0.controls.resize(num_controls, num_times);
    it0.controls.setRandom();

    it0.state_names = {"a", "b", "c"};
    it0.control_names = {"x", "y"};

    // Serialize.
    const std::string filename = "test_OptimalControlIterate_serialization.csv";
    it0.write(filename);

    // Deserialize.
    OptimalControlIterate it1(filename);

    // Test.
    TROPTER_REQUIRE_EIGEN(it0.time, it1.time, 1e-5);
    TROPTER_REQUIRE_EIGEN(it0.states, it1.states, 1e-5);
    TROPTER_REQUIRE_EIGEN(it0.controls, it1.controls, 1e-5);

    REQUIRE(it0.state_names == it1.state_names);
    REQUIRE(it0.control_names == it1.control_names);
}

TEST_CASE("Interpolating an initial guess") {

    using namespace Eigen;
    auto vec = [](const std::vector<double>& v) {
        RowVectorXd ev = Map<const RowVectorXd>(v.data(), v.size());
        return ev;
    };

    // We create an initial guess with 5 time points and upsample it.
    SECTION("Upsampling") {
        OptimalControlIterate it0;
        int num_times = 5;
        int num_states = 2;
        int num_controls = 3;
        it0.time.resize(num_times);
        it0.time << 0, 1, 2, 3, 5; // non-uniform.

        it0.states.resize(num_states, num_times);
        it0.states << 0, 1, 4, 9,81,
                      5, 4, 3, 2, 1;

        it0.controls.resize(num_controls, num_times);
        it0.controls << -1,   0, -1,  0, -1,
                         0,   3, -3,  1,  1,
                         5,   3,  3,  3,  3;

        it0.state_names = {"alpha", "beta"};
        it0.control_names = {"gamma", "rho", "phi"};

        // Upsampling.
        OptimalControlIterate it1 = it0.interpolate(9);
        REQUIRE(it1.state_names == it0.state_names);
        REQUIRE(it1.control_names == it0.control_names);
        TROPTER_REQUIRE_EIGEN(it1.time,
                vec({0, 0.625, 1.25, 1.875, 2.5, 3.125, 3.75, 4.375, 5}),
                1e-15);
        TROPTER_REQUIRE_EIGEN(it1.states.row(0),
                vec({0, 0.625, 1.75, 3.625, 6.5, 13.5, 36, 58.5, 81}), 1e-15);
        TROPTER_REQUIRE_EIGEN(it1.states.row(1),
                vec({5, 4.375, 3.75, 3.125, 2.5, 1.9375, 1.625, 1.3125, 1}),
                1e-15);

        TROPTER_REQUIRE_EIGEN(it1.controls.row(0),
                vec({-1, -.375, -.25, -.875, -.5, -.0625, -.375, -.6875, -1}),
                1e-15);
        TROPTER_REQUIRE_EIGEN(it1.controls.row(1),
                vec({0, 1.875, 1.5, -2.25, -1, 1, 1, 1, 1}), 1e-15);
        TROPTER_REQUIRE_EIGEN(it1.controls.row(2),
                vec({5, 3.75, 3, 3, 3, 3, 3, 3, 3}), 1e-15);

        SECTION("Requesting the same number of points") {
            // We use it0 here.
            auto it2 = it0.interpolate(5);
            REQUIRE(it2.state_names == it0.state_names);
            REQUIRE(it2.control_names == it0.control_names);
            TROPTER_REQUIRE_EIGEN(it2.time, it0.time, 1e-15);
            TROPTER_REQUIRE_EIGEN(it2.states, it0.states, 1e-15);
            TROPTER_REQUIRE_EIGEN(it2.controls, it0.controls, 1e-15);
        }
    }

    // Re back to the original number of points; should recover it0.
    SECTION("Roundtrip") {
        OptimalControlIterate it0;
        int num_times = 5;
        int num_states = 2;
        int num_controls = 3;
        it0.time.resize(num_times);
        it0.time << 0, 1, 2, 3, 4;
        it0.states.resize(num_states, num_times);
        it0.states << 0, 1, 4, 9,81,
                      5, 4, 3, 2, 1;
        it0.controls.resize(num_controls, num_times);
        it0.controls << -1,   0, -1,  0, -1,
                         0,   3, -3,  1,  1,
                         5,   3,  3,  3,  3;
        auto it1 = it0.interpolate(9);
        auto it2 = it1.interpolate(5);
        REQUIRE(it2.state_names == it0.state_names);
        REQUIRE(it2.control_names == it0.control_names);
        TROPTER_REQUIRE_EIGEN(it2.time, it0.time, 1e-15);
        TROPTER_REQUIRE_EIGEN(it2.states, it0.states, 1e-15);
        TROPTER_REQUIRE_EIGEN(it2.controls, it0.controls, 1e-15);
    }

    SECTION("Original times must be sorted") {
        OptimalControlIterate it;
        it.time.resize(5);
        it.time << 0, 1, 2, 1.5, 3;
        REQUIRE_THROWS_WITH(it.interpolate(20),
                Contains("Expected time to be non-decreasing"));
    }

}