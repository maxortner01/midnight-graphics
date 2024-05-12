
std::shared_ptr<mn::Graphics::Model> generate_plane_mesh(mn::Math::Vec2i chunk_index)
{
    using namespace mn;
    using namespace mn::Graphics;

    Model::Frame frame;

    // The current offset in world space calculated from the 2D chunk index
    const auto offset = Math::Vec2f({ Math::x(chunk_index) * CHUNK_SIZE, Math::y(chunk_index) * CHUNK_SIZE });

    for (int32_t y = -1 * CHUNK_CELLS; y <= CHUNK_CELLS; y++)
        for (int32_t x = -1 * CHUNK_CELLS; x <= CHUNK_CELLS; x++)
        {
            // Convert the 2D vertex index to a position in world space
            const auto x_pos = x * CELL_WIDTH + Math::x(offset);
            const auto y_pos = y * CELL_WIDTH + Math::y(offset);

            // Calculate the height from the SimplexNoise
            const auto h = [x_pos, y_pos]() 
            {
                auto r = abs(noise.fractal(8, x_pos * 0.015, y_pos * 0.015) * 4.f);
                r *= r;
                return r;
            }();

            // Push the vertex
            frame.vertices.push_back(Model::Vertex {
                .position = { x_pos, h, y_pos },
                .color = { 1.f, 1.f, 1.f, 1.f }
            });

            // Generate the trianges and store the vertex indices
            if (x < CHUNK_CELLS && y < CHUNK_CELLS)
            {
                const auto X = x + CHUNK_CELLS;
                const auto Y = y + CHUNK_CELLS;
                
                const auto row_size = CHUNK_CELLS * 2 + 1;
                const auto index = Y * row_size + X;
                const auto next_index = (Y + 1) * row_size + X;

                frame.indices.push_back(index);
                frame.indices.push_back(index + 1);
                frame.indices.push_back(next_index + 1);

                frame.indices.push_back(index);
                frame.indices.push_back(next_index + 1);
                frame.indices.push_back(next_index);
            }
        }

    // The normals should be aggregated across all the vertices, then normalized (for smoothed lighting)
    std::vector<Math::Vec3f> normals(frame.vertices.size());
    for (uint32_t i = 0; i < frame.indices.size() / 3; i++)
    {
        const auto triangle = Math::Vec3u({ frame.indices[i * 3 + 0], frame.indices[i * 3 + 1], frame.indices[i * 3 + 2] });

        const auto v1 = frame.vertices[triangle.c[1]].position - frame.vertices[triangle.c[0]].position;
        const auto v2 = frame.vertices[triangle.c[2]].position - frame.vertices[triangle.c[0]].position;
        const auto normal = Math::normalized(Math::outer(v1, v2));

        for (uint32_t j = 0; j < 3; j++)
            normals[triangle.c[j]] += normal;
    }
    for (auto& v : normals) v = Math::normalized(v) * -1.f;
    for (uint32_t i = 0; i < frame.vertices.size(); i++) frame.vertices[i].normal = normals[i];

    return std::make_shared<Model>(Model::fromFrame(frame));
}
