(set-logic QF_NRA)
(declare-fun x () Real)
(declare-fun y () Real)
(declare-fun x1 () Real)
(declare-fun x2 () Real)
(declare-fun y1 () Real)
(declare-fun y2 () Real)
(assert (and (= (- (+ (* x1 x1) (* y1 y1)) 1) 0) (= (- (+ (* (- x2 10) (- x2 10)) (* y2 y2)) 9) 0) (= (* 2 x) (+ x1 x2)) (= (* 2 y) (+ y1 y2))))
(eliminate-quantifiers (exists x1 y1 x2 y2))
(exit)
