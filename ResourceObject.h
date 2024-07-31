class ResourceObject{
	public:
	ResourceObject(ID3D12Resource*resource:resource_)(resource)
	{}
	~ResourceObject() {
		if (resource_) {
			resource_->Release();
		}
	}
	ID3D12Resource* Get() { retrun resource_; }
private:
	ID3D12Resource* resource_;
};

