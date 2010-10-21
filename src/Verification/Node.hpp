#ifndef NODE_HPP_
#define NODE_HPP_

#include "../Core/SymMarking.hpp"

namespace VerifyTAPN
{
	enum Color { WAITING, PASSED, COVERED };

	class Node{
	public:
		Node(const SymMarking& marking, Color color) : marking(marking), color(color) {};
		inline Color GetColor() const { return color; };
		inline SymMarking& GetMarking() { return marking; };
	public:
		inline void Recolor(Color newColor) { color = newColor; }
	private:
		SymMarking marking;
		Color color;
	};
}

#endif /* NODE_HPP_ */