World = {
    Workers = 1,
    ChunkCells = 50,
    CellWidth = 0.4,
    Radius = {}
};

for z = -1,1 do
    for y = -1,1 do
        for x = -1,1 do
            World.Radius[#World.Radius+1] = { x, y, z }
        end
    end
end

