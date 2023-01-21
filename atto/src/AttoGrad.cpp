#include "AttoGrad.h"

#include <iostream>
#include <vector>

namespace atto
{
    class Value {
    public:
        Value() : op(0), label(0), grad(0), value(0) {}
        
        Value(double value, const std::vector<Value>& children = {}, char op = ' ') : value(value), children(children), op(op), grad(0.0), label(0) {
        
        }
        
        Value operator+(const Value& other) const {
            return Value(value + other.value, {*this, other}, '+');
        }
        
        Value operator*(const Value& other) const {
            return Value(value * other.value, { *this, other }, '*');
        }

        void Print() {
            std::cout << "Value{" << label  << "} = " << value << ", Grad = " << grad << ", Op = " << op << std::endl;
        }

        char op;
        char label;
        double value;
        double grad;
        std::vector<Value> children;
        
    };

    void DoTinyGrad() {
        Value L1;
        Value L2;
        double h = 0.01;
        {
            Value a = Value(2.0); a.label = 'a';
            Value b = Value(-3.0); b.label = 'b';
            Value c = Value(10.0); c.label = 'c';
            Value e = a * b; e.label = 'e';
            Value d = e + c; d.label = 'd';
            Value f = Value(-2.0); f.label = 'f';
            L1 = d * f; L1.label = 'L';
        }

        {
            Value a = Value(2.0); a.label = 'a';
            Value b = Value(-3.0); b.label = 'b';
            Value c = Value(10.0); c.label = 'c';
            Value e = a * b; e.label = 'e';
            Value d = e + c; d.label = 'd';
            Value f = Value(-2.0); f.label = 'f';
            L2 = d * f; L2.label = 'L';
            L2.value += h;
        }

        double r = (L2.value - L1.value) / h;

        std::cout.precision(20);
        std::cout << r << std::endl;
        
 
        system("PAUSE");
    }
}