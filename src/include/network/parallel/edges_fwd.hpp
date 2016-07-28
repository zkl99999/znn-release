//
// Copyright (C) 2012-2015  Aleksandar Zlateski <zlateski@mit.edu>
// ---------------------------------------------------------------
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#include "edge.hpp"
#include "nodes.hpp"
#include "../filter.hpp"
#include "../../options/options.hpp"
#include "../../utils/waiter.hpp"
#include "../../utils/task_manager.hpp"
#include "../../initializator/initializators.hpp"
#include "../trivial/utils.hpp"

namespace znn { namespace v4 { namespace parallel_network {

class edges
{
public:
    struct filter_tag {};
    struct dummy_tag {};
    struct max_pooling_tag {};
    struct real_pooling_tag {};
    struct dropout_tag {};
    struct nodeout_tag {};
    struct crop_tag {};
    struct concat_tag {};
    struct split_tag {};
    struct softmax_tag {};
    struct maxout_tag {};
    struct multiply_tag {};
    struct normalize_tag {};
    struct L2_norm_tag {};
    struct scale_tag {};

protected:
    options                                options_;
    waiter                                 waiter_ ;
    std::vector<std::unique_ptr<edge>>     edges_  ;
    std::vector<std::shared_ptr<filter>>   filters_;
    vec3i                                  size_   ;
    task_manager &                         tm_     ;

public:
    edges( nodes *, nodes *, options const &, vec3i const &, task_manager &,
           bool, filter_tag );

    edges( nodes *, nodes *, options const &, task_manager &, dummy_tag );

    edges( nodes *, nodes *, options const &, vec3i const &, task_manager &,
           max_pooling_tag );

    edges( nodes *, nodes *, options const &,task_manager &,
           real_pooling_tag );

    edges( nodes *, nodes *, options const &, task_manager &, phase phs,
           dropout_tag );

    edges( nodes *, nodes *, options const &, task_manager &, phase phs,
           nodeout_tag );

    edges( nodes *, nodes *, options const &, task_manager &, crop_tag );

    edges( nodes *, nodes *, options const &, task_manager &, concat_tag );

    edges( nodes *, nodes *, options const &, task_manager &, split_tag );

    edges( nodes *, nodes *, options const &, task_manager &, softmax_tag );

    edges( nodes *, nodes *, options const &, task_manager &, maxout_tag );

    edges( nodes *, nodes *, options const &, task_manager &, multiply_tag );

    edges( nodes *, nodes *, options const &, task_manager &, phase phs,
           normalize_tag );

    edges( nodes *, nodes *, options const &, task_manager &, L2_norm_tag );

    edges( nodes *, nodes *, options const &, task_manager &, scale_tag );

    ~edges()
    {
        if ( options_.contains("shared") )
        {
            auto name = options_.require_as<std::string>("shared");
            filter::shared_filters_pool.erase(name);
        }
    }

    std::string name() const
    {
        return options_.require_as<std::string>("name");
    }

    void setup()
    {
        for ( auto & e: edges_ )
            e->setup();
    }

    // [kisuklee]
    // This is only temporary implementation and will be removed.
    void set_phase( phase phs )
    {
        for ( auto & e: edges_ )
            e->set_phase(phs);
    }

    void set_eta( real eta )
    {
        if ( filters_.size() )
        {
            options_.push("eta", eta);
            for ( auto & f: filters_ ) f->eta() = eta;
        }
    }

    void set_momentum( real mom )
    {
        if ( filters_.size() )
        {
            options_.push("momentum", mom);
            for ( auto & f: filters_ ) f->momentum() = mom;
        }
    }

    void set_weight_decay( real wd )
    {
        if ( filters_.size() )
        {
            options_.push("weight_decay", wd);
            for ( auto & f: filters_ ) f->weight_decay() = wd;
        }
    }

    void set_patch_size( real s )
    {
        if ( filters_.size() )
        {
            for ( auto & e: edges_ ) e->set_patch_size(s);
        }
    }

    options serialize() const
    {
        options ret = options_;
        if ( filters_.size() )
        {
            if ( !ret.contains("size") )
            {
                ret.push("size", size_);
            }
            ret.push("filters", save_filters(filters_, size_));
        }
        return ret;
    }

    virtual void debug_info() const
    {
        std::cout << "[nodes: " << nodes::name() << "]\n";

        if ( filters_.size() )
        {
            // mean absolute weight
            real s = 0.0;
            size_t count = 0;
            for ( size_t i = 0; i < edges_.size(); ++i )
            {
                if ( edges_[i]->is_enabled() )
                {
                    auto r = abs(filters_[i]->W());
                    s += sum(*r);
                    ++count;
                }
            }
            real val = s/count;
            std::cout << "\tmean absolute weight: " << val << "\n";

            // mean absolute weight gradient
            s = 0.0;
            count = 0;
            for ( size_t i = 0; i < edges_.size(); ++i )
            {
                if ( edges_[i]->is_enabled() )
                {
                    auto r = abs(*filters_[i]->dEdW());
                    s += sum(*r);
                    ++count;
                }
            }
            val = s/count;
            std::cout << "\tmean absolute weight gradient: " << val << "\n";
        }
    }

    void edge_zapped()
    {
        waiter_.one_done();
    }

    void zap()
    {
        for ( auto & e: edges_ )
        {
            e->zap(this);
        }
        waiter_.wait();
    }

};

}}} // namespace znn::v4::parallel_network
