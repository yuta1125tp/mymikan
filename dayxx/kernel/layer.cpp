#include "layer.hpp"
#include <algorithm>

Layer::Layer(unsigned int id) : id_{id} {}

unsigned int Layer::ID() const { return id_; }

Layer &Layer::SetWindow(const std::shared_ptr<Window> &window)
{
    window_ = window;
    return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const { return window_; }
Vector2D<int> Layer::GetPosition() const { return pos_; }

Layer &Layer::Move(Vector2D<int> pos)
{
    pos_ = pos;
    return *this;
}
Layer &Layer::MoveRelative(Vector2D<int> pos_diff)
{
    pos_ += pos_diff;
    return *this;
}

void Layer::DrawTo(FrameBuffer &screen, const Rectangle<int> &area) const
{
    if (window_)
    {
        window_->DrawTo(screen, pos_, area);
    }
}

void LayerManager::SetWriter(FrameBuffer *screen)
{
    screen_ = screen;
    FrameBufferConfig back_config = screen->Config();
    back_config.frame_buffer = nullptr;
    back_buffer_.Initialize(back_config);
}

Layer &LayerManager::NewLayer()
{
    latest_id_++;
    // emplace_backは追加した要素の参照を返すが、std::unique_ptr<Layer>&は共有できないので、Layer&型に変換している
    return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw(const Rectangle<int> &area) const
{
    for (auto layer : layer_stack_)
    {
        layer->DrawTo(back_buffer_, area);
    }
    screen_->Copy(area.pos, back_buffer_, area);
}

void LayerManager::Draw(unsigned int id) const
{
    bool draw = false;
    Rectangle<int> window_area;
    // layer_stack_の画面奥側から順に調べて、再描画対象より手前の重複エリアを差描画する。
    for (auto layer : layer_stack_)
    {
        if (layer->ID() == id)
        {
            window_area.size = layer->GetWindow()->Size();
            window_area.pos = layer->GetPosition();
            draw = true;
        }
        if (draw)
        {
            layer->DrawTo(back_buffer_, window_area);
        }
    }
    screen_->Copy(window_area.pos, back_buffer_, window_area);
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_pos)
{
    // FindLayerはidが見つからないときにNullptrを返す。IDの有効性の確認は呼び出し側の責任 みかん本218p
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move(new_pos);
    Draw({old_pos, window_size});
    Draw(id);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff)
{
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->MoveRelative(pos_diff);
    Draw({old_pos, window_size});
    Draw(id);
}

void LayerManager::UpDown(unsigned int id, int new_height)
{
    if (new_height < 0)
    {
        Hide(id);
        return;
    }

    if (new_height > layer_stack_.size())
    {
        new_height = layer_stack_.size();
    }

    auto layer = FindLayer(id);
    auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    auto new_pos = layer_stack_.begin() + new_height;

    if (old_pos == layer_stack_.end())
    {
        layer_stack_.insert(new_pos, layer);
        return;
    }

    if (new_pos == layer_stack_.end())
    {
        --new_pos;
    }

    layer_stack_.erase(old_pos);
    layer_stack_.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id)
{
    auto layer = FindLayer(id);
    auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
    if (pos != layer_stack_.end())
    {
        // layer_stack_から取り除くことで非表示にする
        layer_stack_.erase(pos);
    }
}

Layer *LayerManager::FindLayer(unsigned int id)
{
    // ラムダ式で述語(pred)を定義している。
    // [id]はキャプチャでラムダ式の外側のローカル変数をラムダ式内部で使うための仕組み
    // みかん本218p
    auto pred = [id](const std::unique_ptr<Layer> &elem)
    {
        return elem->ID() == id;
    };

    auto it = std::find_if(layers_.begin(), layers_.end(), pred);
    if (it == layers_.end())
    {
        return nullptr;
    }

    return it->get();
}

LayerManager *layer_manager;
