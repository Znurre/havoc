# HAVOC - Heap Allocated Value Object Containers

## What is HAVOC, and what is the use case?

### Scenario

You have a node tree, possibly an AST, which you want to express as a C++ object structure. If dealing with an AST, you want to be able to express it as closely as possible to your grammar. This probably means using either polymorphism or something like `std::variant` to express a choice between multiple nodes.

For example, if we're talking about a traditional C like language, you might have "binary expressions" and "unary expressions", both being "expressions" in your grammar. 

You could either represent them like this (the traditional approach)...

```cpp
struct expression
{
    virtual ~expression() = default;
};

struct binary_expression : expression
{
};

struct unary_expression : expression
{
};
```
...or like this:

```cpp
struct binary_expression
{
};

struct unary_expression
{
};

using expression = std::variant<binary_expression, unary_expression>;
```

I would argue that the later approach is preferable in a number of ways:

#### You enforce non-nullability in your contract

If an expression is expected to be present according to the grammar, then this should be reflected in your C++ code as well. Using a polymorphic type would mean exposing it as an `std::unique_ptr<expression>`, for example, which can have a null value.

Using an `std::variant` here instead enforces that there can never be a null value.

#### You can easily implement visitors without relying on RTTI, and without sacrificing flexibility

Using the polymorphic approach, you would either need to implement a visitor by using RTTI/`dynamic_cast`, or you would need to use vtables. Both approaches would end up performing the same as HAVOC, but would require a lot of boilerplate.

If you are familiar with the visitor pattern, you would likely go for the vtables solution. Here we would let `expression` have a method (commonly named `accept` in the visitor pattern), which would invoke the correct method of the visitor based on the concrete type of the object. Obviously, this approach would **not** let us vary the return type in C++, as the `accept` method cannot be both templated and virtual. We then end up with bad compromises such as having our visitor implementations store state, or returning `std::any` like ANTLR does.

Once again, using an `std::variant` here together with `std::visit` will work around this problem.

#### Your contract better mirrors the grammar

I would argue that having something like...

```cpp
using expression = std::variant<binary_expression, unary_expression>;
```
...perfectly mirrors a grammar where an expression can be *either* a unary expression, or a binary expression.

And while object oriented programmers might be more used to working with inheritence, I think it would be hard to argue that it provides the same overview and 1:1 mapping to the grammar in this scenario.

### The problem

Okay, now we've talked about how good `std::variant` is for this use case, so where is the problem? Well, obviously this example was extremely simplified. In reality, a unary expression would consist of an operand and another expression, while a binary expression would consist of a left expression, an operand, and a right expression. This is where we start getting into issues with `std::variant`, as we end up creating a recursive dependency - not only to the variant definition, but also in terms of allocations. The non-nullability aspect of `std::variant` comes to bite us, as we would literally create a singularity of allocations.

We could work around this by having `expression` be a type alias for `std::variant<std::unique_ptr<binary_expression>, std::unique_ptr<unary_expression>>`, but then we reintroduce one of the problem we tried to solve. All of a sudden, we have nullability in our node tree again. In addition, we can no longer treat the expressions as value objects. We will need to do allocations using `new` or `std::make_unique`.

The same problem occurs with `std::optional`. We just want to have our node tree mirror our grammar, and sometimes, a node is optional in the grammar. This should be reflected in the code as well. Using `std::optional` for this feels like a great choice, but once again we end up causing a singularity of allocations. It could be replaced by `std::unique_ptr`, and one could even argue that this would be reasonable since this is a case where we actually *want* the nullability. However, once again it would make things clumsier to work with. If you are like me, you want to work with value objects, with value semantics, and the fact that a value is *optional* should not change that fact.

### The solution

It quickly becomes apparent that what we want is actually something like `std::variant` and `std::optional`, but backed by dynamically allocated memory. This way, there are no memory allocation singularities, and no issues with recursion.

This is where HAVOC comes into the picture. It provides a type called `one_of`, which is basically an `std::variant`, and a type called `optional` which is... well, `std::optional`. Both have value semantics, but uses `std::unique_ptr` behind the scenes to allocate and manage their storage.

The `one_of` type even provides transparent, lazy instantiation of a value in case you try to access a non-initialized instance. Basically, the intention is to have it act and appear as if it was an `std::variant` - but without any of the caveats that we mentioned above. No nullability (unless you wrap it in an `optional` ofc), no issues with recursion. Only happiness.

## Using it

HAVOC is shipped as a single C++20 compliant header file, and can simply be included and used. There are some differences to how the containers work compared to `std::variant` and `std::optional`.

The easiest one is `optional`, where the only difference is that it has a reduced API. Basically, I only implemented whatever I needed for my use case, but it could easily be extended if needed.

