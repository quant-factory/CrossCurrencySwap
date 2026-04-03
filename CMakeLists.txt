cmake_minimum_required(VERSION 4.2)
project(CrossCurrencySwap)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(CrossCurrencySwap main.cpp
        Core/Dates.cpp
        Core/Dates.h
        Core/numerics.h
        Core/Types.cpp
        Core/Types.h
        Market/Produits.cpp
        Market/Produits.h
        Market/YieldCurve.cpp
        Market/YieldCurve.h
        Pricers/CCSPricer.cpp
        Pricers/CCSPricer.h
        Pricers/sensibilities.h
        Pricers/sensibilities.cpp)
