use std::fmt;
use std::fmt::{Display, Formatter};
use cxx;

type AtomType = String;

pub trait CloneBoxLTL {
    fn clone_box(&self) -> Box<dyn LTLFormula>;
}

pub trait LTLFormula: Display + CloneBoxLTL + 'static { }

impl<U: LTLFormula + Clone> CloneBoxLTL for U {
    fn clone_box(&self) -> Box<dyn LTLFormula> {
        Box::new(self.clone())
    }
}

#[derive(Clone)]
pub struct AtomicProposition {
    var: AtomType
}

impl Display for AtomicProposition {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        self.var.fmt(f)
    }
}
impl LTLFormula for AtomicProposition {}


pub struct Conjunction {
    lhs: Box<dyn LTLFormula>,
    rhs: Box<dyn LTLFormula>,
}

impl Display for Conjunction {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "({} ∧ {})", self.lhs.as_ref(), self.rhs.as_ref())
    }
}
impl LTLFormula for Conjunction {}

impl Clone for Conjunction {
    fn clone(&self) -> Self {
        Conjunction {
            lhs: self.lhs.clone_box(),
            rhs: self.rhs.clone_box(),
        }
    }
}

pub struct Eventually {
    inner: Box<dyn LTLFormula>
}
impl Display for Eventually {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "<>{}", self.inner.as_ref())
    }
}
impl Clone for Eventually {
    fn clone(&self) -> Self {
        Eventually { inner: self.inner.clone_box() }
    }
}
impl LTLFormula for Eventually {}



pub struct Disjunction {
    lhs: Box<dyn LTLFormula>,
    rhs: Box<dyn LTLFormula>,
}
impl Display for Disjunction {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "({} ∨ {})", self.lhs.as_ref(), self.rhs.as_ref())
    }
}
impl Clone for Disjunction {
    fn clone(&self) -> Self {
        Disjunction {
            lhs: self.lhs.clone_box(),
            rhs: self.rhs.clone_box(),
        }
    }
}
impl LTLFormula for Disjunction {}

pub struct Negation {
    inner: Box<dyn LTLFormula>,
}
impl Display for Negation {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "¬{}", self.inner.as_ref())
    }
}
impl Clone for Negation {
    fn clone(&self) -> Self {
        Negation { inner: self.inner.clone_box() }
    }
}
impl LTLFormula for Negation {}


pub struct Always {
    inner: Box<dyn LTLFormula>,
}
impl Display for Always {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "[]{}", self.inner.as_ref())
    }
}
impl Clone for Always {
    fn clone(&self) -> Self {
        Always { inner: self.inner.clone_box() }
    }
}
impl LTLFormula for Always {}


pub(crate) fn atomic(var: AtomType) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(AtomicProposition { var }))
}

pub(crate) fn conjunction(lhs: Box<Box<dyn LTLFormula>>, rhs: Box<Box<dyn LTLFormula>>) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(Conjunction { lhs: lhs.clone_box(), rhs: rhs.clone_box() }))
}

pub(crate) fn eventually(inner: Box<Box<dyn LTLFormula>>) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(Eventually { inner: inner.clone_box() }))
}

pub(crate) fn disjunction(lhs: Box<Box<dyn LTLFormula>>, rhs: Box<Box<dyn LTLFormula>>) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(Disjunction { lhs: lhs.clone_box(), rhs: rhs.clone_box() }))
}

pub(crate) fn negation(inner: Box<Box<dyn LTLFormula>>) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(Negation { inner: inner.clone_box() }))
}

pub(crate) fn always(inner: Box<Box<dyn LTLFormula>>) -> Box<Box<dyn LTLFormula>> {
    Box::new(Box::new(Always { inner: inner.clone_box() }))
}

type DynLTLFormula = Box<dyn LTLFormula>;

#[cxx::bridge(namespace=ltl)]
mod ltl_ffi {
    extern "Rust" {
        type DynLTLFormula;
        type AtomicProposition;
        type Conjunction;
        type Eventually;
        type Disjunction;
        type Negation;
        type Always;
        fn atomic(var: String) -> Box<DynLTLFormula>;
        fn conjunction(lhs: Box<DynLTLFormula>, rhs: Box<DynLTLFormula>) -> Box<DynLTLFormula>;
        fn eventually(eventually: Box<DynLTLFormula>) -> Box<DynLTLFormula>;
        fn disjunction(lhs: Box<DynLTLFormula>, rhs: Box<DynLTLFormula>) -> Box<DynLTLFormula>;
        fn negation(inner: Box<DynLTLFormula>) -> Box<DynLTLFormula>;
        fn always(inner: Box<DynLTLFormula>) -> Box<DynLTLFormula>;

        fn to_string(self: &DynLTLFormula) -> String;
    }
}
