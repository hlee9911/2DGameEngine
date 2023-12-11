-- This is Lua global variable
some_variable = 7 * 6

-- This is lua table for initialization of our game engine
config = {
	title = "My Game Engine",
	fullscreen = false,
	resolution = {
		width = 800,
		height = 600
	}
}

entities = {
	tank = {
		transform = {
			position = {
				x = 300,
				y = 200
			},
			rotation = 0,
			scale = {
				x = 1,
				y = 1
			}
		},
		boxcollider = {

		}
	}
}

-- This is lua function that calculte the factorial of a number
function factorial(n)
	i = n;
	result = 1;
	while i > 0 do
		result = result * i;
		i = i - 1;
	end
	-- print("Factorial of " .. n .. " is " .. result);
	return result;
end

print("hello" .. add(5, 7))


-- good for soft tasks
-- 1. basic game logic
-- 2. movement decision
-- 3. simple collision resposnen