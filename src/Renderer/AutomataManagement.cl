#pragma OPENCL EXTENSION cl_khr_3d_image_writes : enable

enum CellType
{
	CellTypeNone,
	CellTypeWater,
	CellTypeMetal,
	CellTypeEarth,
};
typedef enum CellType CellType;

uint16 EncodeCell(CellType type, uint16 payload)
{
	//payload must only use the lower 12 bits
	payload <<= 4;
	payload |= (uint8) type;
	return payload;
}

void DecodeCell(uint16 encoded, CellType* type, uint16* payload)
{
	//payload must only use the lower 12 bits
	* type = (CellType) (encoded & 0x000Fu);
	encoded >>= 4;
	* payload = encoded;
}

kernel void UpdateState(global image3d_t initialState, global image3d_t finalState)
{
	size_t x = get_global_id(0);
	size_t y = get_global_id(1);
	size_t z = get_global_id(2);
	size_t w = get_global_size(0);
	size_t h = get_global_size(1);
	size_t d = get_global_size(2);
	if (x >= w) return;
	if (y >= h) return;
	if (z >= d) return;
	write_imageui(state, (int4){x, y, z, 0}, (int4){0, 0, 0, 0});
	
}