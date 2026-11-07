/* Static showcase: no framework, trackers, external fonts or build-time runtime. */
(() => {
  'use strict';
  const data = window.CYGAME_SITE;
  const shared = data.shared;
  const guidePage = document.body.dataset.page === 'guide';
  let language = 'en', selectedMode = 0, gallery = [], galleryIndex = 0, returnFocus, chapterObserver;
  const escapeHTML = (s) => String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const e = escapeHTML;
  const lines = s => e(s).replace(/\n/g, '<br>');
  const arrow = '<span aria-hidden="true">↗</span>';
  const github = '<svg aria-hidden="true" viewBox="0 0 24 24"><path fill="currentColor" d="M12 2a10 10 0 0 0-3.16 19.49c.5.09.68-.22.68-.48v-1.86c-2.78.6-3.37-1.18-3.37-1.18-.45-1.15-1.11-1.46-1.11-1.46-.91-.62.07-.61.07-.61 1 .07 1.53 1.03 1.53 1.03.89 1.52 2.34 1.08 2.91.83.09-.65.35-1.08.64-1.33-2.22-.25-4.55-1.11-4.55-4.94 0-1.09.39-1.98 1.03-2.68-.1-.25-.45-1.27.1-2.65 0 0 .84-.27 2.75 1.02a9.59 9.59 0 0 1 5 0c1.91-1.29 2.75-1.02 2.75-1.02.55 1.38.2 2.4.1 2.65.64.7 1.03 1.59 1.03 2.68 0 3.84-2.34 4.69-4.57 4.94.36.31.68.92.68 1.85v2.73c0 .27.18.58.69.48A10 10 0 0 0 12 2Z"/></svg>';
  const doc = (id='start') => `guide.html?lang=${language}#${id}`;
  const source = file => `${shared.repo}/blob/main/${file}`;
  const screenshotPath = name => `assets/screenshots/${name}.png`;
  const heading = (k,t,b='') => `<div class="section-heading"><p class="eyebrow">${e(k)}</p><h2>${lines(t)}</h2>${b ? `<p class="lead">${e(b)}</p>` : ''}</div>`;
  const list = items => `<ul class="feature-list">${items.map(x=>`<li>${e(x)}</li>`).join('')}</ul>`;
  function shot(name, alt, hero=false) {
    return `<button class="screenshot${hero?' hero-shot':''}" data-image="${e(name)}" data-caption="${e(alt)}" aria-label="${e(data[language].enlarge+': '+alt)}"><img src="${screenshotPath(name)}" alt="${e(alt)}" width="1584" height="961" ${hero?'fetchpriority="high"':'loading="lazy"'} decoding="async"></button>`;
  }
  const cards = items => `<div class="image-grid">${items.map(c=>`<article class="image-card">${shot(c.image,c.alt)}<div class="card-copy"><h3>${e(c.title)}</h3><p>${e(c.body)}</p></div></article>`).join('')}</div>`;
  function header(c) {
    return `<a class="skip" href="#main">${e(c.skip)}</a><header class="site-header"><div class="header-inner"><a class="brand" href="index.html?lang=${language}" aria-label="CyGameSDK"><img src="assets/mark.svg" width="31" height="31" alt=""><span>CyGame<span class="brand-light">SDK</span></span></a><nav class="primary-nav" aria-label="${e(c.features)}"><a href="${guidePage?'index.html?lang='+language+'#workspace':'#workspace'}">${e(c.features)}</a><a href="${doc()}">${e(c.documentation)}</a></nav><div class="header-actions"><div class="language-switch" role="group" aria-label="${e(c.language)}"><button data-lang="en" aria-pressed="${language==='en'}" lang="en">EN</button><button data-lang="fr" aria-pressed="${language==='fr'}" lang="fr">FR</button></div><a class="button github-button" href="${shared.repo}">${github}<span>GitHub</span>${arrow}</a><a class="button primary header-download" href="${guidePage?'index.html?lang='+language+'#download':'#download'}">${e(c.download)}${arrow}</a><button class="mobile-menu-button" data-menu aria-label="${e(c.menu)}" aria-expanded="false" aria-controls="mobile-menu"><span aria-hidden="true">☰</span></button></div></div><nav id="mobile-menu" class="mobile-menu" aria-label="${e(c.menu)}" hidden><a href="${guidePage?'index.html?lang='+language+'#workspace':'#workspace'}">${e(c.features)}</a><a href="${doc()}">${e(c.documentation)}</a><a href="${guidePage?'index.html?lang='+language+'#download':'#download'}">${e(c.download)}</a></nav></header>`;
  }
  function footer(c) {
    return `<footer class="site-footer wrap"><div><a class="brand" href="index.html?lang=${language}"><img src="assets/mark.svg" width="27" height="27" alt="">CyGameSDK</a><p>${e(c.footer)}</p><small>© 2026 CyGameSDK</small></div><nav aria-label="${e(c.documentation)}"><a href="${doc()}">${e(c.documentation)}</a><a href="${shared.repo}">GitHub ${arrow}</a><a href="${shared.release}">${e(c.releaseButton)}</a><a href="${source('LICENSE')}">${e(c.license)}</a><a href="#top">${e(c.top)} ↑</a></nav></footer>`;
  }
  function showcase(c) {
    return `<main id="main">
      <section class="hero wrap" aria-labelledby="product-title"><div class="hero-intro"><div class="hero-identity"><p class="release-line"><span class="status-dot"></span>${e(c.status)}</p><h1 id="product-title">CyGame<span>SDK</span><span class="title-period">.</span></h1><p class="hero-line">${e(c.heroLine)}</p></div><div class="hero-description"><p>${e(c.heroBody)}</p><div class="actions"><a class="button primary" href="${shared.download}">↓ ${e(c.download)}</a><a class="text-link" href="${doc()}">${e(c.documentation)} ${arrow}</a></div><p class="meta">${e(c.heroMeta)}</p></div></div><figure class="hero-figure"><div class="screen-frame"><div class="screen-bar"><span class="window-dots" aria-hidden="true">● ● ●</span><span>CyGameSDK / Sandbox</span><span aria-hidden="true">↗</span></div>${shot('workspace',c.heroAlt,true)}</div><figcaption>${e(c.heroCaption)}</figcaption></figure></section>
      <div class="tech-strip wrap">${c.strip.map((x,i)=>`<span><span class="strip-symbol" aria-hidden="true">${['⌘','◈','{ }'][i]}</span>${e(x)}</span>`).join('')}</div>
      <nav class="chapter-nav" aria-label="${e(c.chapterLabel)}"><div class="wrap">${c.chapters.map(([id,label])=>`<a href="#${id}">${e(label)}</a>`).join('')}</div></nav>
      <section class="intro section wrap">${heading(c.introKicker,c.introTitle)}<div class="intro-copy"><p class="lead">${e(c.introBody)}</p><div class="intro-notes">${c.introNotes.map(n=>`<div><span class="tiny-cross" aria-hidden="true">+</span><h3>${e(n[0])}</h3><p>${e(n[1])}</p></div>`).join('')}</div></div></section>
      <section id="workspace" class="section wrap">${heading(c.workspaceKicker,c.workspaceTitle,c.workspaceBody)}<div class="mode-tabs" role="tablist" aria-label="${e(c.workspaceKicker)}">${c.modes.map((m,i)=>`<button role="tab" id="mode-${i}" data-mode="${i}" aria-selected="${i===selectedMode}" aria-controls="mode-panel" tabindex="${i===selectedMode?'0':'-1'}"><span class="tab-number">0${i+1}</span>${e(m.label)}</button>`).join('')}</div><div id="mode-panel" role="tabpanel" aria-labelledby="mode-${selectedMode}"></div></section>
      <section id="graphs" class="section feature-section wrap"><div class="split-heading">${heading(c.graphs.kicker,c.graphs.title)}<div><p class="lead">${e(c.graphs.body)}</p>${list(c.graphs.bullets)}<a class="text-link" href="${doc('explore')}">${e(c.graphs.link)} ${arrow}</a></div></div>${shot('logic',c.graphs.alt)}${cards(c.graphCards)}</section>
      <section id="materials" class="section material-section"><div class="wrap"><div class="split-heading">${heading(c.materials.kicker,c.materials.title)}<div><p class="lead">${e(c.materials.body)}</p>${list(c.materials.bullets)}</div></div>${shot('materials',c.materials.alt)}<p class="caption-note">${e(c.materials.note)}</p>${cards(c.materialCards)}</div></section>
      <section id="assets" class="section wrap">${heading(c.assets.kicker,c.assets.title,c.assets.body)}${cards(c.assetCards)}<div class="portable"><div><p class="eyebrow">${e(c.portableKicker)}</p><h3>${lines(c.portableTitle)}</h3></div><div><p>${e(c.portableBody)}</p><div class="format-row"><span>.cy3d</span><span>GLB</span><span>.cypack</span></div><a class="text-link" href="${source('app/Docs/CommonAssetFormat.md')}">${e(c.portableLink)} ${arrow}</a></div></div></section>
      <section id="automation" class="section automation-section"><div class="wrap"><div class="split-heading">${heading(c.automation.kicker,c.automation.title)}<div><p class="lead">${e(c.automation.body)}</p><a class="text-link" href="${doc('mcp')}">${e(c.automation.link)} ${arrow}</a></div></div><div class="sdk-flow" aria-label="MCP / EditorCore"><div>GUI</div><span aria-hidden="true">↘</span><div class="flow-core">EditorCore</div><span aria-hidden="true">↗</span><div>SDK / MCP</div></div><div class="three-steps">${c.automation.steps.map((s,i)=>`<article><span class="step-number">0${i+1}</span><h3>${e(s[0])}</h3><p>${e(s[1])}</p></article>`).join('')}</div><p class="caption-note">${e(c.automation.note)}</p><div class="automation-proof">${shot('diagnostics',c.automation.alt)}</div></div></section>
      <section id="extend" class="section wrap"><div class="split-heading">${heading(c.extend.kicker,c.extend.title)}<div><p class="lead">${e(c.extend.body)}</p><a class="text-link" href="${doc('adapter')}">${e(c.guideNav[4][1])} ${arrow}</a></div></div><div class="three-steps integration-steps">${c.integrationSteps.map(s=>`<article><span class="step-number">${e(s[0])}</span><h3>${e(s[1])}</h3><p>${e(s[2])}</p></article>`).join('')}</div>${shot('build',c.extend.alt)}<p class="caption-note">${e(c.extend.note)}</p></section>
      <section class="section personalize wrap"><div>${heading(c.personalize.kicker,c.personalize.title,c.personalize.body)}</div>${shot('theme-light',c.personalize.alt)}</section>
      <section class="section docs-section wrap">${heading(c.docsKicker,c.docsTitle,c.docsBody)}<div class="actions"><a class="button primary" href="${doc()}">${e(c.docsButton)} ${arrow}</a><a class="text-link" href="${source('AI_GUIDE.md')}">${e(c.docsSecondary)} ${arrow}</a></div></section>
      <section class="section faq wrap"><h2>${e(c.faqTitle)}</h2><div>${c.faqs.map(([q,a])=>`<details><summary>${e(q)}<span aria-hidden="true">+</span></summary><p>${e(a)}</p></details>`).join('')}</div></section>
      <section id="download" class="download-section wrap"><div><p class="eyebrow">CYGAMESDK / 0.29.0</p><h2>${e(c.downloadTitle)}</h2><p class="lead">${e(c.downloadBody)}</p><div class="actions"><a class="button primary" href="${shared.download}">↓ ${e(c.sourceButton)}</a><a class="text-link" href="${shared.release}">${e(c.releaseButton)} ${arrow}</a></div><p class="meta">${e(c.downloadNote)}</p></div><div class="download-mark" aria-hidden="true"><img src="assets/mark.svg" alt="" width="150" height="150"></div></section>
    </main>`;
  }
  function guide(c) {
    return `<main id="main" class="wrap guide-layout"><aside class="guide-sidebar"><a class="text-link" href="index.html?lang=${language}">← ${e(c.back)}</a><p class="eyebrow">CYGAMESDK / 0.29.0</p><nav aria-label="${e(c.chapterLabel)}">${c.guideNav.map(([id,t])=>`<a href="#${id}">${e(t)}</a>`).join('')}</nav></aside><article class="guide-article"><header><p class="release-line">${e(c.status)}</p><h1>${e(c.guideTitle)}</h1><p class="lead">${e(c.guideIntro)}</p></header>${c.guideSections.map(s=>`<section id="${s.id}"><h2>${e(s.title)}</h2><p>${e(s.body)}</p>${s.code?`<pre><code>${e(s.code)}</code></pre>`:''}${list(s.items)}<a class="text-link" href="${source('app/Docs/'+s.ref)}">${e(c.technical)} ${arrow}</a></section>`).join('')}</article></main>`;
  }
  function modal(c) {
    return `<dialog class="lightbox" aria-label="${e(c.enlarge)}"><div class="lightbox-toolbar"><p id="lightbox-caption"></p><button class="icon-button" data-close aria-label="${e(c.close)}">${e(c.close)} <kbd>Esc</kbd><span aria-hidden="true">×</span></button></div><div class="lightbox-stage"><button class="icon-button lightbox-prev" data-previous aria-label="${e(c.previous)}">←</button><img alt=""><button class="icon-button lightbox-next" data-next aria-label="${e(c.next)}">→</button></div><div class="lightbox-footer"><span id="lightbox-count"></span><span>${e(c.escape)}</span></div></dialog>`;
  }
  function setMode(index, focus=false) {
    selectedMode = index;
    const c=data[language], m=c.modes[index], panel=document.getElementById('mode-panel');
    document.querySelectorAll('[data-mode]').forEach((t,i)=>{t.setAttribute('aria-selected',String(i===index));t.tabIndex=i===index?0:-1;});
    panel.setAttribute('aria-labelledby',`mode-${index}`);
    panel.innerHTML=`<div class="mode-copy"><span class="mono-count">0${index+1} / 04</span><h3>${e(m.title)}</h3><p>${e(m.body)}</p><ul class="tag-list">${m.tags.map(t=>`<li>${e(t)}</li>`).join('')}</ul><a class="text-link" href="${doc('explore')}">${e(c.documentation)} ${arrow}</a></div>${shot(m.image,m.alt)}`;
    if(focus) document.getElementById(`mode-${index}`).focus();
  }
  function showImage(index) {
    galleryIndex=(index+gallery.length)%gallery.length;
    const item=gallery[galleryIndex], dlg=document.querySelector('.lightbox'), img=dlg.querySelector('img');
    img.src=screenshotPath(item.name);img.alt=item.caption;
    document.getElementById('lightbox-caption').textContent=item.caption;
    document.getElementById('lightbox-count').textContent=`${data[language].imageOf} ${galleryIndex+1} / ${gallery.length}`;
  }
  function closeImage() { const dlg=document.querySelector('.lightbox');if(dlg.open)dlg.close(); }
  function render() {
    const c=data[language]; chapterObserver?.disconnect(); document.documentElement.lang=language;
    document.title=guidePage?c.guideTitle:c.title;
    document.querySelector('meta[name="description"]').content=c.description;
    document.getElementById('app').innerHTML=header(c)+(guidePage?guide(c):showcase(c))+footer(c)+modal(c);
    if(!guidePage)setMode(selectedMode);
    const dlg=document.querySelector('.lightbox');
    dlg.addEventListener('close',()=>{document.body.classList.remove('modal-open');returnFocus?.focus();});
    dlg.addEventListener('click',ev=>{if(ev.target===dlg||ev.target.classList.contains('lightbox-stage'))closeImage();});
    dlg.addEventListener('keydown',ev=>{if(ev.key==='ArrowRight'){ev.preventDefault();showImage(galleryIndex+1);}if(ev.key==='ArrowLeft'){ev.preventDefault();showImage(galleryIndex-1);}});
    if('IntersectionObserver' in window && !guidePage){
      chapterObserver=new IntersectionObserver(entries=>{for(const entry of entries)if(entry.isIntersecting){document.querySelectorAll('.chapter-nav a').forEach(a=>{if(a.hash==='#'+entry.target.id)a.setAttribute('aria-current','location');else a.removeAttribute('aria-current');});}}, {rootMargin:'-20% 0px -55% 0px'});
      document.querySelectorAll('main > section[id]').forEach(s=>chapterObserver.observe(s));
    }
  }
  document.addEventListener('click',ev=>{
    const menu=ev.target.closest('[data-menu]');
    if(menu){const nav=document.getElementById('mobile-menu');nav.hidden=!nav.hidden;menu.setAttribute('aria-expanded',String(!nav.hidden));return;}
    if(ev.target.closest('#mobile-menu a')){document.getElementById('mobile-menu').hidden=true;document.querySelector('[data-menu]').setAttribute('aria-expanded','false');}
    const lang=ev.target.closest('[data-lang]');
    if(lang){language=lang.dataset.lang;try{localStorage.setItem('cygame-site-language',language);}catch{}const u=new URL(location.href);u.searchParams.set('lang',language);history.replaceState(null,'',u);render();document.querySelector(`[data-lang="${language}"]`).focus();return;}
    const tab=ev.target.closest('[data-mode]');if(tab){setMode(Number(tab.dataset.mode));return;}
    const image=ev.target.closest('[data-image]');if(image){returnFocus=image;gallery=[...document.querySelectorAll('[data-image]')].map(b=>({name:b.dataset.image,caption:b.dataset.caption}));showImage([...document.querySelectorAll('[data-image]')].indexOf(image));document.querySelector('.lightbox').showModal();document.body.classList.add('modal-open');return;}
    if(ev.target.closest('[data-close]'))closeImage();
    if(ev.target.closest('[data-previous]'))showImage(galleryIndex-1);
    if(ev.target.closest('[data-next]'))showImage(galleryIndex+1);
  });
  document.addEventListener('keydown',ev=>{if(ev.key==='Escape'&&!document.getElementById('mobile-menu').hidden){document.getElementById('mobile-menu').hidden=true;const b=document.querySelector('[data-menu]');b.setAttribute('aria-expanded','false');b.focus();}if(!ev.target.matches('[data-mode]'))return;let i=selectedMode;if(ev.key==='ArrowRight')i=(i+1)%4;else if(ev.key==='ArrowLeft')i=(i+3)%4;else if(ev.key==='Home')i=0;else if(ev.key==='End')i=3;else return;ev.preventDefault();setMode(i,true);});
  const requested=new URLSearchParams(location.search).get('lang');
  try {language=requested||(localStorage.getItem('cygame-site-language')||'en');}catch{language=requested||'en';}
  if(!['en','fr'].includes(language))language='en';
  render();
  if(location.hash) requestAnimationFrame(()=>document.getElementById(decodeURIComponent(location.hash.slice(1)))?.scrollIntoView({behavior:'instant'}));
})();
