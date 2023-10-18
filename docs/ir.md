# Overview of Sea Of Nodes Intermediate Representation (SoN IR)

There are 3 type of instructions:
1) Control
2) Data
3) Hybrid

## 1) Control

It is instruction which inputs are only control instructions. Сontrol instructions show the direction of execution.

It is:
* Start
* Region

## 2) Data

It is instruction whitch doesn't have control instructions in inputs. It containe some value or result of some calculation.

It is:
* Add
* Mul
* Constant
* Parameter
e.t.c

## 3) Hybrid

It is instruction which in `input0` have control instruction and in others have data instructions (or data instructions may not exist).

It is:
* If
* Call
* Return
* Compare
* Phi
