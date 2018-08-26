//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace std {
namespace experimental {
namespace fundamentals_v3 {
namespace detail {

template<size_t R>
struct extents_dynamic ;

template<>
struct extents_dynamic<1> {
  using type = extents<dynamic_extent> ;
};
template<>
struct extents_dynamic<2> {
  using type = extents<dynamic_extent,dynamic_extent> ;
};
template<>
struct extents_dynamic<3> {
  using type = extents<dynamic_extent,dynamic_extent,dynamic_extent> ;
};

template<class Slice, class Enable = void> struct is_slice_range ;

template<class T>
struct is_slice_range< T, typename enable_if< is_integral<T>::value >::type > : public false_type {};

// assuming size == 2
template<class T>
struct is_slice_range< initializer_list<T>, typename enable_if< is_integral<T>::value >::type > : public true_type {};

template<class ElementType,
         class Extents,
         class LayoutPolicy,
         class Accessor,
         class ... SliceSpecifiers>
struct subspan_deduction {

  static_assert( sizeof...(SliceSpecifiers) == Extents::rank() , "" );

	static constexpr size_t sum() noexcept { return 0 ; }

	template<class... Flag>
	static constexpr size_t sum( size_t i , Flag... flag ) noexcept
	  { return i + sum( flag... ); }

  enum : size_t { Rank = sum( is_slice_range<SliceSpecifiers>::value... ) };

  using layout = typename
	  conditional< is_same<LayoutPolicy,layout_left>::value ||
	                    is_same<LayoutPolicy,layout_right>::value ||
	                    is_same<LayoutPolicy,layout_stride>::value ,
										  layout_stride , void >::type ;

	using type = basic_mdspan<ElementType, typename extents_dynamic<Rank>::type, layout, Accessor > ;
};

template<class ExtentsNew, class ExtentsOld, class ... SliceSpecifiers>
struct compose_new_extents;

template<ptrdiff_t ... ExtentsNew, ptrdiff_t E0, ptrdiff_t ... ExtentsOld, class ... SliceSpecifiers>
struct compose_new_extents<extents<ExtentsNew...>,extents<E0,ExtentsOld...>,all_type,SliceSpecifiers...> {
  typedef compose_new_extents<extents<ExtentsNew...,E0>,extents<ExtentsOld...>,SliceSpecifiers...> next_compose_new_extents;
  typedef typename next_compose_new_extents::extents_type extents_type; 

  template<class OrgExtents, class ... DynamicExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e,  
                        array<ptrdiff_t,OrgExtents::rank()>& strides, ptrdiff_t& offset,
                                                   all_type, SliceSpecifiers...s, DynamicExtents...de) {
    strides[sizeof...(ExtentsNew)] = strides[OrgExtents::rank()-sizeof...(SliceSpecifiers)-1];
    return next_compose_new_extents::create_sub_extents(e,strides,offset,s...,de...);
  }
};
template<ptrdiff_t ... ExtentsNew, ptrdiff_t ... ExtentsOld, class ... SliceSpecifiers>
struct compose_new_extents<extents<ExtentsNew...>,extents<dynamic_extent,ExtentsOld...>,all_type,SliceSpecifiers...> {
  typedef compose_new_extents<extents<ExtentsNew...,dynamic_extent>,extents<ExtentsOld...>,SliceSpecifiers...> next_compose_new_extents;
  typedef typename next_compose_new_extents::extents_type extents_type; 

  template<class OrgExtents, class ... DynamicExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e,  
                        array<ptrdiff_t,OrgExtents::rank()>& strides, ptrdiff_t& offset,
                                                   all_type, SliceSpecifiers...s, DynamicExtents...de) {
    strides[sizeof...(ExtentsNew)] = strides[OrgExtents::rank()-sizeof...(SliceSpecifiers)-1];
    return next_compose_new_extents::create_sub_extents(e,strides,offset,s...,de...,e.extent(OrgExtents::rank()-sizeof...(SliceSpecifiers)-1));
  }
};

template<ptrdiff_t ... ExtentsNew, ptrdiff_t E0, ptrdiff_t ... ExtentsOld, class IT, class ... SliceSpecifiers>
struct compose_new_extents<extents<ExtentsNew...>,extents<E0,ExtentsOld...>,pair<IT,IT>,SliceSpecifiers...> {
  typedef compose_new_extents<extents<ExtentsNew...,dynamic_extent>,extents<ExtentsOld...>,SliceSpecifiers...> next_compose_new_extents; 
  typedef typename next_compose_new_extents::extents_type extents_type; 

  template<class OrgExtents, class ... DynamicExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e, array<ptrdiff_t,OrgExtents::rank()>& strides, ptrdiff_t& offset,
                                                   pair<IT,IT> p, SliceSpecifiers ... s, DynamicExtents...de) {
    strides[sizeof...(ExtentsNew)] = strides[OrgExtents::rank()-sizeof...(SliceSpecifiers)-1];
    offset += p.first*strides[OrgExtents::rank()-sizeof...(SliceSpecifiers)-1];
    return next_compose_new_extents::create_sub_extents(e,strides,offset,s...,de...,ptrdiff_t(p.second-p.first));
  }
};

template<ptrdiff_t ... ExtentsNew, ptrdiff_t E0, ptrdiff_t ... ExtentsOld, class ... SliceSpecifiers>
struct compose_new_extents<extents<ExtentsNew...>,extents<E0,ExtentsOld...>,ptrdiff_t,SliceSpecifiers...> {
  typedef compose_new_extents<extents<ExtentsNew...>,extents<ExtentsOld...>,SliceSpecifiers...> next_compose_new_extents;
  typedef typename next_compose_new_extents::extents_type extents_type; 

  template<class OrgExtents, class ... DynamicExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e, array<ptrdiff_t,OrgExtents::rank()>& strides, ptrdiff_t& offset, 
                                                   const ptrdiff_t v, SliceSpecifiers...s,DynamicExtents...de) {
    offset += v*strides[OrgExtents::rank()-sizeof...(SliceSpecifiers)-1];
    return next_compose_new_extents::create_sub_extents(e,strides,offset,s...,de...);
  }
};

template<ptrdiff_t ... ExtentsNew>
struct compose_new_extents<extents<ExtentsNew...>,extents<>> {
  typedef extents<ExtentsNew...> extents_type; 

  template<class OrgExtents, class ... DynamicExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e, array<ptrdiff_t,OrgExtents::rank()>, ptrdiff_t, DynamicExtents...de) {
    return extents_type(de...);
  }
  template<class OrgExtents>
  static constexpr extents_type create_sub_extents(const OrgExtents e, array<ptrdiff_t,OrgExtents::rank()>, ptrdiff_t) {
    return extents_type();
  }
};


template<class Extents, class...SliceSpecifiers>
struct subspan_deduce_extents {
  typedef typename compose_new_extents<extents<>,Extents,SliceSpecifiers...>::extents_type extents_type;
  typedef array<ptrdiff_t,Extents::rank()> stride_type;
  static constexpr extents_type create_sub_extents(const Extents e,stride_type& strides, ptrdiff_t& offset, SliceSpecifiers...s) {
    return compose_new_extents<extents<>,Extents,SliceSpecifiers...>::create_sub_extents(e,strides,offset,s...);
  }
};

}

template<class ElementType, class Extents, class LayoutPolicy,
           class AccessorPolicy, class... SliceSpecifiers>
    basic_mdspan<ElementType, typename detail::subspan_deduce_extents<Extents,SliceSpecifiers...>::extents_type,
                 layout_stride, typename AccessorPolicy::offset_policy >
      subspan(const basic_mdspan<ElementType, Extents, LayoutPolicy, AccessorPolicy>& src, SliceSpecifiers ... slices) noexcept {
    typedef typename detail::subspan_deduce_extents<Extents,SliceSpecifiers...>::extents_type sub_extents_type;
    typedef typename AccessorPolicy::offset_policy sub_accessor_policy;
    typedef basic_mdspan<ElementType,sub_extents_type,layout_stride,sub_accessor_policy> sub_mdspan_type;

    array<ptrdiff_t,Extents::rank()> strides;
    for(size_t r = 0; r<Extents::rank(); r++)
      strides[r] = src.stride(r);

    ptrdiff_t offset = 0;
    sub_extents_type sub_extents = detail::subspan_deduce_extents<Extents,SliceSpecifiers...>::create_sub_extents(src.extents(),strides,offset,slices...);

    array<ptrdiff_t,sub_extents_type::rank()> sub_strides;
    for(size_t r = 0; r<sub_extents_type::rank(); r++)
      sub_strides[r] = strides[r];

    typename AccessorPolicy::offset_policy::pointer ptr = src.accessor().offset(src.data(),offset);    
    return sub_mdspan_type(ptr,sub_extents,sub_strides);
  }

}}}
