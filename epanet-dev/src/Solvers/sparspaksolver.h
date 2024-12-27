/* EPANET 3
 *
 * Copyright (c) 2016 Open Water Analytics
 * Licensed under the terms of the MIT License (see the LICENSE file for details).
 *
 */

//! \file sparspaksolver.h
//! \brief Description of the SparspakSolver class.

#ifndef SPARSPAKSOLVER_H_
#define SPARSPAKSOLVER_H_

#include "matrixsolver.h"




//! \class SparspakSolver
//! \brief Solves Ax = b using the SPARSPAK routines.
//!
//! This class is derived from the MatrixSolver class and provides an
//! interface to the SPARSPAK routines, originally developed by George
//! and Liu, for re-ordering, factorizing, and solving via Cholesky
//! decomposition a sparse, symmetric, positive definite set of linear
//! equations Ax = b.

class SparspakSolver: public MatrixSolver
{
  public:

    // Constructor/Destructor

    SparspakSolver(std::ostream& logger);
    ~SparspakSolver();

    // Methods

    int    init(int nrows, int nnz, int* xrow, int* xcol);
    void   reset();

    double getDiag(int i);
    double getOffDiag(int i);
    double getRhs(int i);

    void   setDiag(int i, double a);
    void   setRhs(int i, double b);
    void   addToDiag(int i, double a);
    void   addToOffDiag(int j, double a);
    void   addToRhs(int i, double b);
    int    solve(int n, double x[]);

//! Serialize to JSON for SparspakSolver
nlohmann::json to_json() const override {
    return {
        {"nrows", nrows},
        {"nnz", nnz},
        {"nnzl", nnzl},
        {"perm", perm ? nlohmann::json(std::vector<int>(perm, perm + nrows)) : nlohmann::json(nullptr)},
        {"invp", invp ? nlohmann::json(std::vector<int>(invp, invp + nrows)) : nlohmann::json(nullptr)},
        {"xlnz", xlnz ? nlohmann::json(std::vector<int>(xlnz, xlnz + nrows + 1)) : nlohmann::json(nullptr)},
        {"xnzsub", xnzsub ? nlohmann::json(std::vector<int>(xnzsub, xnzsub + nrows + 1)) : nlohmann::json(nullptr)},
        {"nzsub", nzsub ? nlohmann::json(std::vector<int>(nzsub, nzsub + nnzl)) : nlohmann::json(nullptr)},
        {"xaij", xaij ? nlohmann::json(std::vector<int>(xaij, xaij + nnz)) : nlohmann::json(nullptr)},
        {"lnz", lnz ? nlohmann::json(std::vector<double>(lnz, lnz + nnzl)) : nlohmann::json(nullptr)},
        {"diag", diag ? nlohmann::json(std::vector<double>(diag, diag + nrows)) : nlohmann::json(nullptr)},
        {"rhs", rhs ? nlohmann::json(std::vector<double>(rhs, rhs + nrows)) : nlohmann::json(nullptr)},
        {"temp", temp ? nlohmann::json(std::vector<double>(temp, temp + nrows)) : nlohmann::json(nullptr)}
    };
}

//! Deserialize from JSON for SparspakSolver
void from_json(const nlohmann::json& j) override {
    nrows = j.at("nrows").get<int>();
    nnz = j.at("nnz").get<int>();
    nnzl = j.at("nnzl").get<int>();

    if (!j.at("perm").is_null()) {
        std::copy(j.at("perm").get<std::vector<int>>().begin(), j.at("perm").get<std::vector<int>>().end(), perm);
    }

    if (!j.at("invp").is_null()) {
        std::copy(j.at("invp").get<std::vector<int>>().begin(), j.at("invp").get<std::vector<int>>().end(), invp);
    }

    if (!j.at("xlnz").is_null()) {
        std::copy(j.at("xlnz").get<std::vector<int>>().begin(), j.at("xlnz").get<std::vector<int>>().end(), xlnz);
    }

    if (!j.at("xnzsub").is_null()) {
        std::copy(j.at("xnzsub").get<std::vector<int>>().begin(), j.at("xnzsub").get<std::vector<int>>().end(), xnzsub);
    }

    if (!j.at("nzsub").is_null()) {
        std::copy(j.at("nzsub").get<std::vector<int>>().begin(), j.at("nzsub").get<std::vector<int>>().end(), nzsub);
    }

    if (!j.at("xaij").is_null()) {
        std::copy(j.at("xaij").get<std::vector<int>>().begin(), j.at("xaij").get<std::vector<int>>().end(), xaij);
    }

    if (!j.at("lnz").is_null()) {
        std::copy(j.at("lnz").get<std::vector<double>>().begin(), j.at("lnz").get<std::vector<double>>().end(), lnz);
    }

    if (!j.at("diag").is_null()) {
        std::copy(j.at("diag").get<std::vector<double>>().begin(), j.at("diag").get<std::vector<double>>().end(), diag);
    }

    if (!j.at("rhs").is_null()) {
        std::copy(j.at("rhs").get<std::vector<double>>().begin(), j.at("rhs").get<std::vector<double>>().end(), rhs);
    }

    if (!j.at("temp").is_null()) {
        std::copy(j.at("temp").get<std::vector<double>>().begin(), j.at("temp").get<std::vector<double>>().end(), temp);
    }
}

  private:

    int     nrows;    // number of rows in system Ax = b
    int     nnz;      // number of non-zero off-diag. coeffs. in A
    int     nnzl;     // number of non-zero off-diag. coeffs. in factorized matrix L
    int*    perm;     // permutation of rows in A
    int*    invp;     // inverse row permutation
    int*    xlnz;     // index vector for non-zero entries in L
    int*    xnzsub;   // index vector for entries of nzsub
    int*    nzsub;    // column indexes for non-zero entries in each row of L
    int*    xaij;     // maps off-diag. coeffs. of A to lnz
    int*    link;     // work array
    int*    first;    // work array
    double* lnz;      // off-diag. coeffs. of factorized matrix L
    double* diag;     // diagonal coeffs. of A
    double* rhs;      // right hand side vector
    double* temp;     // work array
    std::ostream& msgLog;
};

#endif
