World = {
    Workers = 3,
    ChunkCells = 40,
    CellWidth = 0.35,
    Radius = {}
};

for z = -2,2 do
    for y = -2,2 do
        for x = -2,2 do
            World.Radius[#World.Radius+1] = { x, y, z }
        end
    end
end

