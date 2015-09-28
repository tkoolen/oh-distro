classdef MultipleTreeProblem
  
  properties
    trees
    startPoints
    nTrees
    status
    mergingThreshold
    iterations
  end
  
  methods
    
    function obj = MultipleTreeProblem(trees, startPoints, varargin)
      opt = struct('mergingthreshold', 0.2);
      optNames = fieldnames(opt);
      nArgs = length(varargin);
      if round(nArgs/2)~=nArgs/2
        error('Needs propertyName/propertyValue pairs')
      end
      for pair = reshape(varargin,2,[])
        inpName = lower(pair{1});
        if any(strcmp(inpName,optNames))
          opt.(inpName) = pair{2};
        else
          error('%s is not a recognized parameter name',inpName)
        end
      end
      
      obj.trees = trees;
      obj.startPoints = startPoints;
      obj.nTrees = numel(trees);
      obj.status = Status.EXPLORING;
      obj.mergingThreshold = opt.mergingthreshold;
    end
    
    function [obj, info, cost, qPath, times] = rrt(obj, xStart, xGoal, options)
      if nargin < 4, options = struct(); end
      global tree
      
      times.reaching = 0;
      times.improving = 0;
      times.checking = 0;
      reachingTimer = tic;
      %Set and apply defaults
      defaultOptions.N = 1000;
      defaultOptions.visualize = true;
      defaultOptions.firstFeasibleTraj = false;
      options = applyDefaults(options, defaultOptions);
      
      [~, startTree] = ismember(xStart', obj.startPoints', 'rows');
      [~, goalTree] = ismember(xGoal', obj.startPoints', 'rows');
      if startTree ~= 1
        obj.startPoints = [obj.startPoints(:, startTree) obj.startPoints(:,1:obj.nTrees ~= startTree)];
        obj.trees = [obj.trees(startTree) obj.trees(1:obj.nTrees ~= startTree)];
      end
      if goalTree ~= 2
        obj.startPoints = [obj.startPoints(:, [1 goalTree]) obj.startPoints(:,[false 2:obj.nTrees ~= goalTree])];
        obj.trees = [obj.trees([1 startTree]) obj.trees(2:obj.nTrees ~= goalTree)];
      end
      
      
      assert(obj.trees(1).isValid(obj.startPoints(:, 1)))
      assert(obj.trees(2).isValid(obj.startPoints(:, 2)))
      for treeIdx = obj.nTrees:-1:3
        if ~obj.trees(treeIdx).isValid(obj.startPoints(:, treeIdx))
          obj = obj.deleteTree(treeIdx);
        end
      end
      for treeIdx = 1:obj.nTrees
          obj.trees(treeIdx).costType = options.costType;
          obj.trees(treeIdx) = obj.trees(treeIdx).init(obj.startPoints(:, treeIdx));
      end
      
      %Iteration loop
      obj.iterations = zeros(1, obj.nTrees);
      for iter = 1:options.N
        
        %Check if the goal has been reached
        xLast = obj.trees(treeIdx).getVertex(obj.trees(treeIdx).n);
        if obj.status ~= Status.GOAL_REACHED
          [d, idxNearest, treeIdxNearest] = nearestNeighbor(obj, xLast, treeIdx);
          %         drawTreePoints(obj.trees(treeIdx).n, 'tree', obj.trees(treeIdx), 'frame', true, 'text', 'LastAddedPoint');
          %         drawTreePoints(idNearest, 'tree', obj.trees(treeNearest), 'frame', true, 'text', 'ClosestPoint');
          xNearest = obj.trees(treeIdxNearest).getVertex(idxNearest);
          if d < obj.mergingThreshold && obj.trees(treeIdxNearest).CollisionFree(xLast, xNearest)
            if any(treeIdx == [1 2])
              [obj, prevRoot] = obj.mergeTrees(idxNearest, obj.trees(treeIdx).n, treeIdxNearest, treeIdx, options);
            else
              [obj, prevRoot] = obj.mergeTrees(obj.trees(treeIdx).n, idxNearest, treeIdx, treeIdxNearest, options);
            end
            %If the merged trees are the start and goal tree
            if all(ismember([treeIdx, treeIdxNearest], [1, 2]))
              obj.status = Status.GOAL_REACHED;
              obj.trees(1) = obj.trees(1).getTrajToGoal(prevRoot);
              obj.trees(1).eta = 0.5;
              lastCost = obj.trees(1).C(obj.trees(1).traj(1));
              lastUpdate = obj.trees(1).n;
              times.reaching = toc(reachingTimer);
              improvingTimer = tic;
            end
          end
        end
        
        %if goal has been reached
        if obj.status == Status.GOAL_REACHED
          %if the cost improves recompute trajectory
          if obj.trees(1).C(obj.trees(1).traj(1)) < lastCost
            obj.trees(1) = obj.trees(1).getTrajToGoal(obj.trees(1).traj(1));
            lastCost = obj.trees(1).C(obj.trees(1).traj(1));
            lastUpdate = obj.trees(1).n;
          end
%           fprintf('n_points: %d\t\tpath Length: %.4f\n', obj.trees(1).n, obj.trees(1).C(obj.trees(1).traj(1)))
          %break the loop if the cost doesn't improve after having added
          %10 points
          if options.firstFeasibleTraj || lastUpdate > 0 && obj.trees(1).n - lastUpdate > 10
            times.improving = toc(improvingTimer);
            shortcut = tic;
            info = Info(Info.SUCCESS);
            break
          end
          treeIdx = 1;
        else
          [~, treeIdx] = min([obj.trees.n]);
          xGoals = NaN(obj.trees(1).num_vars, obj.nTrees);
          for i = find(1:obj.nTrees ~= treeIdx)
            xGoals(:,i) = obj.trees(i).getCentroid();
          end
          xGoals(:,treeIdx) = [];
%           xGoals = obj.startPoints(:, 1:obj.nTrees ~= treeIdx);
          %         drawTreePoints(xGoal, 'frame', true)
        end
        
        obj.iterations(treeIdx) = obj.iterations(treeIdx) + 1;
        tree = treeIdx;
%         fprintf('Current Tree %d\n', treeIdx)
%         drawTreePoints(obj.startPoints(:,treeIdx), 'text', 'StartVertex', 'pointsize', 0.02, 'colour', [0 1 0]);
        if obj.status == Status.GOAL_REACHED
          obj.trees(treeIdx) = rrtStarIteration(obj.trees(treeIdx), obj.status, xGoals, obj.iterations(treeIdx), floor(20/obj.nTrees));
        else
          obj.trees(treeIdx) = rrtIteration(obj.trees(treeIdx), obj.status, xGoals, obj.iterations(treeIdx), floor(20/obj.nTrees));
        end
%         fprintf('Iteration %d: Tree %d vertices %d\n', iter, treeIdx, obj.trees(treeIdx).n)
        %update the tree plot
        if options.visualize
          obj.trees(treeIdx) = obj.trees(treeIdx).drawTree();
          drawnow
        end
      end
      if obj.status == Status.GOAL_REACHED
        obj.trees(1) = obj.trees(treeIdx);
        obj.trees(1).setLCMGL(obj.trees(1).lcmgl_name, obj.trees(1).line_color);
        obj.trees(1) = obj.trees(1).shortcut();
        times.shortcut = toc(shortcut);
        rebuildTimer = tic;
        qPath = obj.trees(1).rebuildTraj(xStart, xGoal);
        cost = obj.trees(1).C(obj.trees(1).traj(1));
        if options.visualize
          fprintf('Final Cost = %.4f\n', cost)
        end
      else
        info = Info(Info.FAIL_TOO_MANY_ITERATIONS);
        qPath = [];
      end
      times.rebuild = toc(rebuildTimer);
    end
    
    function [obj, prevRoot] = mergeTrees(obj, ptAidx, ptBidx, treeAidx, treeBidx, options)
      %Merge tree A into B and delete tree A
      lastAdded = NaN;
      nextToAdd = ptAidx;
      nextParent = ptBidx;
      while lastAdded ~= 1
        obj.trees(treeBidx) = obj.trees(treeBidx).addVertex(obj.trees(treeAidx).getVertex(nextToAdd), nextParent);
        if options.visualize
          obj.trees(treeBidx) = obj.trees(treeBidx).drawTree();
        end
        nextParent = obj.trees(treeBidx).n;
        queue = [nextToAdd; nextParent];
        done = nextToAdd;
        while ~isempty(queue)
          idParentA = queue(1, end);
          idParentB = queue(2, end);
          queue = queue(:, 1:end-1);
          children = obj.trees(treeAidx).getChildren(idParentA);
          children = children(children ~= lastAdded);
          for ch = 1:length(children)
            if all(done ~= children(ch))
              obj.trees(treeBidx) = obj.trees(treeBidx).addVertex(obj.trees(treeAidx).getVertex(children(ch)), idParentB);
              if options.visualize
                obj.trees(treeBidx) = obj.trees(treeBidx).drawTree();
              end
              queue = [queue [children(ch); obj.trees(treeBidx).n]];
              done = [done children(ch)];
            end
          end
        end
        lastAdded = nextToAdd;
        nextToAdd = obj.trees(treeAidx).getParentId(lastAdded);
      end
      obj = obj.deleteTree(treeAidx);
      prevRoot = nextParent;
    end
    
    function obj = deleteTree(obj, treeIdx)
      tree = obj.trees(treeIdx);
      tree.setLCMGL(tree.lcmgl_name, tree.line_color);
      obj.trees(treeIdx) = [];
      obj.startPoints(:, treeIdx) = [];
      obj.nTrees = obj.nTrees - 1;
    end
    
    function [d, idNearest, treeNearest] = nearestNeighbor(obj, q, treeIdx)
      d = Inf;
      idNearest = NaN;
      treeNearest = NaN;
      for t = 1:obj.nTrees
        if t ~= treeIdx
          [newd, newId] = obj.trees(t).nearestNeighbor(q);
          if newd < d
            d = newd;
            idNearest = newId;
            treeNearest = t;
          end
        end
      end
    end
    
  end
end