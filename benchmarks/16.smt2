(set-logic QF_NRA)
(declare-fun x () Real)
(declare-fun y () Real)
(declare-fun z () Real)
(assert (and (= (+ (- (+ (* 16 (* x x)) (* 4 (* x (* y y)))) (* 4 z)) 1) 0) (= (+ (+ (* 4 x) (* 2 (* (* y y) z))) 1) 0) (= (- (- (* 2 (* (* x x) z)) x) (* 2 (* y y))) 0)))
(eliminate-quantifiers (exists z) (forall y) (exists x))
(exit)
