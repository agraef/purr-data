pd.post("Hello, universe!")

local lua_test = pd.Class:new():register("lua_test")

function lua_test:initialize(name, atoms)
  pd.post("Hello, world!")
  self.inlets = 1
  self.outlets = 1
  return true
end

function lua_test:finalize()
  pd.post("Bye bye, world!")
end

function lua_test:in_1(sel,atoms)
  for i,v in ipairs(atoms) do
    pd.post(sel.."  " .. i .. " = " .. v)
  end
  self:outlet(1, "got", atoms) -- 2nd arg is an array
end
